if exists('loaded_project_specific')
	finish
endif
let loaded_project_specific = 1

setlocal cindent
setlocal cinoptions=>4,n-2,{2,^-2,:2,=2,g0,h2,p5,t0,+2,(0,u0,w1,m1
setlocal expandtab
setlocal shiftwidth=2
setlocal tabstop=8
setlocal softtabstop=2
setlocal textwidth=100
setlocal fo-=ro fo+=cql

set makeprg=ninja\ -C\ build
