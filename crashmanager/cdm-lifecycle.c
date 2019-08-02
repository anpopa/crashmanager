/* cdm-lifecycle.c
 *
 * Copyright 2019 Alin Popa <alin.popa@fxdata.ro>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written
 * authorization.
 */

#include "cdm-lifecycle.h"

static GSourceFuncs lifecycle_source_funcs =
{
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
};

CdmLifecycle *
cdm_lifecycle_new (void)
{
  CdmLifecycle *lifecycle = g_new0 (CdmLifecycle, 1);

  g_assert (lifecycle);

  g_ref_count_init (&lifecycle->rc);
  g_ref_count_inc (&lifecycle->rc);

  lifecycle->source = g_source_new (&lifecycle_source_funcs, sizeof(GSource));
  g_source_ref (lifecycle->source);

  return lifecycle;
}

CdmLifecycle *
cdm_lifecycle_ref (CdmLifecycle *lifecycle)
{
  g_assert (lifecycle);
  g_ref_count_inc (&lifecycle->rc);
  return lifecycle;
}

void
cdm_lifecycle_unref (CdmLifecycle *lifecycle)
{
  g_assert (lifecycle);

  if (g_ref_count_dec (&lifecycle->rc) == TRUE)
    {
      g_source_unref (lifecycle->source);
      g_free (lifecycle);
    }
}

GSource *
cdm_lifecycle_get_source (CdmLifecycle *lifecycle)
{
  g_assert (lifecycle);
  return lifecycle->source;
}
