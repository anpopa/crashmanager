/*
 * SPDX license identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2019-2020 Alin Popa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * \author Alin Popa <alin.popa@fxdata.ro>
 * \file cdm-transfer.c
 */

#include "cdm-transfer.h"

#ifdef WITH_GENIVI_DLT
#include <dlt.h>
#include <dlt_filetransfer.h>
#endif
#ifdef WITH_SCP_TRANSFER
#include <arpa/inet.h>
#include <libssh2.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#ifdef WITH_GENIVI_DLT
DLT_DECLARE_CONTEXT(cdm_transfer_ctx);
#define DLT_MIN_TIMEOUT 1
#endif

#ifdef WITH_SCP_TRANSFER
#define SCP_BUFFER_SZ 1024
#endif

/**
 * @brief GSource prepare function
 */
static gboolean transfer_source_prepare(GSource *source, gint *timeout);

/**
 * @brief GSource dispatch function
 */
static gboolean transfer_source_dispatch(GSource *source, GSourceFunc callback, gpointer _transfer);

/**
 * @brief GSource callback function
 */
static gboolean transfer_source_callback(gpointer _transfer, gpointer _entry);

/**
 * @brief GSource destroy notification callback function
 */
static void transfer_source_destroy_notify(gpointer _transfer);

/**
 * @brief Async queue destroy notification callback function
 */
static void transfer_queue_destroy_notify(gpointer _transfer);

/**
 * @brief Transfer thread function
 */
static void transfer_thread_func(gpointer _entry, gpointer _transfer);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs transfer_source_funcs = {
    transfer_source_prepare,
    NULL,
    transfer_source_dispatch,
    NULL,
    NULL,
    NULL,
};

static gboolean transfer_source_prepare(GSource *source, gint *timeout)
{
    CdmTransfer *transfer = (CdmTransfer *) source;

    CDM_UNUSED(timeout);

    return (g_async_queue_length(transfer->queue) > 0);
}

static gboolean transfer_source_dispatch(GSource *source, GSourceFunc callback, gpointer _transfer)
{
    CdmTransfer *transfer = (CdmTransfer *) source;
    gpointer entry = g_async_queue_try_pop(transfer->queue);

    CDM_UNUSED(callback);
    CDM_UNUSED(_transfer);

    if (entry == NULL)
        return G_SOURCE_CONTINUE;

    return transfer->callback(transfer, entry) == TRUE ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static gboolean transfer_source_callback(gpointer _transfer, gpointer _entry)
{
    CdmTransfer *transfer = (CdmTransfer *) _transfer;
    CdmTransferEntry *entry = (CdmTransferEntry *) _entry;

    g_assert(transfer);
    g_assert(entry);

    g_debug("Push file to tread pool transfer %s", entry->file_path);
    g_thread_pool_push(transfer->tpool, entry, NULL);

    return TRUE;
}

static void transfer_source_destroy_notify(gpointer _transfer)
{
    CdmTransfer *transfer = (CdmTransfer *) _transfer;

    g_assert(transfer);
    g_debug("Transfer destroy notification");

    cdm_transfer_unref(transfer);
}

static void transfer_queue_destroy_notify(gpointer _transfer)
{
    CDM_UNUSED(_transfer);
    g_debug("Transfer queue destroy notification");
}

#ifdef WITH_SCP_TRANSFER
static void transfer_scp_upload(CdmOptions *options, const gchar *file_path)
{
    g_autofree gchar *username = NULL;
    g_autofree gchar *password = NULL;
    g_autofree gchar *publickey = NULL;
    g_autofree gchar *privatekey = NULL;
    g_autofree gchar *serveraddr = NULL;
    g_autofree gchar *serverpath = NULL;
    g_autofree gchar *file_basename = NULL;
    g_autofree gchar *scppath = NULL;
    LIBSSH2_SESSION *session = NULL;
    LIBSSH2_CHANNEL *channel = NULL;
    struct sockaddr_in sin;
    struct stat fileinfo;
    FILE *local = NULL;
    gchar mem[SCP_BUFFER_SZ];
    size_t nread;
    char *ptr;
    gint sock;
    gint rc;

    serveraddr = cdm_options_string_for(options, KEY_TRANSFER_ADDRESS);
    if (strlen(serveraddr) == 0)
        return;

    rc = libssh2_init(0);
    if (rc != 0) {
        g_warning("Libssh2 initialization failed (%d)", rc);
        return;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        g_warning("Failed to create scp transfer socket");
        return;
    }

    sin.sin_family = AF_INET;
    sin.sin_port = htons((guint16) cdm_options_long_for(options, KEY_TRANSFER_PORT));

    if (inet_pton(AF_INET, serveraddr, &sin.sin_addr.s_addr) <= 0) {
        g_warning("Failed inet_pton error occured");
        return;
    }

    if (connect(sock, (struct sockaddr *) (&sin), sizeof(struct sockaddr_in)) != 0) {
        g_warning("Failed to connect to scp server");
        return;
    }

    session = libssh2_session_init();
    if (!session) {
        g_warning("Failed to create libssh2 session");
        return;
    }

    rc = libssh2_session_handshake(session, sock);
    if (rc) {
        g_warning("Failure establishing SSH session: %d", rc);
        return;
    }

    /* no early return from now on, go to scp_shutdown for cleanup */

    username = cdm_options_string_for(options, KEY_TRANSFER_USER);
    password = cdm_options_string_for(options, KEY_TRANSFER_PASSWORD);
    publickey = cdm_options_string_for(options, KEY_TRANSFER_PUBLIC_KEY);
    privatekey = cdm_options_string_for(options, KEY_TRANSFER_PRIVATE_KEY);

    if (libssh2_userauth_publickey_fromfile(session, username, publickey, privatekey, password)) {
        g_warning("Authentication by public key failed");
        goto scp_shutdown;
    }

    local = fopen(file_path, "rb");
    if (!local) {
        g_warning("Can't open local file for transfer %s", file_path);
        goto scp_shutdown;
    }

    stat(file_path, &fileinfo);

    serverpath = cdm_options_string_for(options, KEY_TRANSFER_PATH);
    file_basename = g_path_get_basename(file_path);
    scppath = g_build_filename(serverpath, file_basename, NULL);

    channel = libssh2_scp_send(
        session, scppath, fileinfo.st_mode & 0777, (unsigned long) fileinfo.st_size);
    if (!channel) {
        gchar *errmsg;
        gint errlen;
        gint err = libssh2_session_last_error(session, &errmsg, &errlen, 0);

        g_warning("Unable to open a session: (%d) %s", err, errmsg);
        goto scp_shutdown;
    }

    do {
        nread = fread(mem, 1, sizeof(mem), local);
        if (nread <= 0)
            break;

        ptr = mem;

        do {
            /* write the same data over and over, until error or completion */
            ssize_t retval = libssh2_channel_write(channel, mem, nread);

            if (retval < 0) {
                g_warning("Transfer error %ld", retval);
                break;
            } else {
                /* retval indicates how many bytes were written this time */
                ptr += retval;
                nread -= (size_t) retval;
            }
        } while (nread);
    } while (TRUE);

    libssh2_channel_send_eof(channel);
    libssh2_channel_wait_eof(channel);
    libssh2_channel_wait_closed(channel);
    libssh2_channel_free(channel);

scp_shutdown:
    if (session) {
        libssh2_session_disconnect(session, "Normal Shutdown");
        libssh2_session_free(session);
    }

    close(sock);

    if (local)
        fclose(local);

    libssh2_exit();
}
#endif

static void transfer_thread_func(gpointer _entry, gpointer _transfer)
{
    g_autofree gchar *file_name = NULL;
    CdmTransferEntry *entry = (CdmTransferEntry *) _entry;
    CdmTransfer *transfer = (CdmTransfer *) _transfer;

    g_assert(entry);
    g_assert(transfer);

    file_name = g_path_get_basename(entry->file_path);

    g_info("Transfer file %s", file_name);

#if defined(WITH_GENIVI_DLT)
    if (dlt_user_log_file_header_alias(&cdm_transfer_ctx, entry->file_path, file_name) == 0) {
        gint pkgcount = dlt_user_log_file_packagesCount(&cdm_transfer_ctx, entry->file_path);
        gint lastpkg = 0;
        gint success = 1;

        while (lastpkg < pkgcount) {
            gint total = 2;
            gint used = 2;
            dlt_user_check_buffer(&total, &used);

            while ((total - used) < (total / 2)) {
                usleep(100 * DLT_MIN_TIMEOUT);
                dlt_user_log_resend_buffer();
                dlt_user_check_buffer(&total, &used);
            }

            lastpkg++;

            if (dlt_user_log_file_data(
                    &cdm_transfer_ctx, entry->file_path, lastpkg, DLT_MIN_TIMEOUT)
                < 0) {
                success = 0;
                break;
            }
        }

        if (success)
            dlt_user_log_file_end(&cdm_transfer_ctx, entry->file_path, 0);
    }
#elif defined(WITH_SCP_TRANSFER)
    transfer_scp_upload(transfer->options, entry->file_path);
#else
    g_debug("No transfer method selected at build time");
#endif

    if (entry->callback)
        entry->callback(entry->user_data, entry->file_path);

    g_free(entry->file_path);
    g_free(entry);
}

CdmTransfer *cdm_transfer_new(CdmOptions *options)
{
    CdmTransfer *transfer
        = (CdmTransfer *) g_source_new(&transfer_source_funcs, sizeof(CdmTransfer));

    g_assert(options);
    g_assert(transfer);

    g_ref_count_init(&transfer->rc);

#ifdef WITH_GENIVI_DLT
    DLT_REGISTER_CONTEXT(cdm_transfer_ctx, "FLTR", "Crashmanager file transfer");
#endif

    transfer->options = cdm_options_ref(options);
    transfer->callback = transfer_source_callback;
    transfer->queue = g_async_queue_new_full(transfer_queue_destroy_notify);
    transfer->tpool = g_thread_pool_new(transfer_thread_func, transfer, 1, TRUE, NULL);

    g_source_set_callback(
        CDM_EVENT_SOURCE(transfer), NULL, transfer, transfer_source_destroy_notify);
    g_source_attach(CDM_EVENT_SOURCE(transfer), NULL);

    return transfer;
}

CdmTransfer *cdm_transfer_ref(CdmTransfer *transfer)
{
    g_assert(transfer);
    g_ref_count_inc(&transfer->rc);
    return transfer;
}

void cdm_transfer_unref(CdmTransfer *transfer)
{
    g_assert(transfer);

    if (g_ref_count_dec(&transfer->rc) == TRUE) {
        g_async_queue_unref(transfer->queue);
#ifdef WITH_GENIVI_DLT
        DLT_UNREGISTER_CONTEXT(cdm_transfer_ctx);
#endif
        cdm_options_unref(transfer->options);
        g_thread_pool_free(transfer->tpool, TRUE, FALSE);
        g_source_unref(CDM_EVENT_SOURCE(transfer));
    }
}

CdmStatus cdm_transfer_file(CdmTransfer *transfer,
                            const gchar *file_path,
                            CdmTransferEntryCallback callback,
                            gpointer user_data)
{
    CdmTransferEntry *entry = NULL;

    g_assert(transfer);
    g_assert(file_path);

    entry = g_new0(CdmTransferEntry, 1);

    entry->file_path = g_strdup(file_path);
    entry->user_data = user_data;
    entry->callback = callback;

    g_async_queue_push(transfer->queue, entry);

    return CDM_STATUS_OK;
}
