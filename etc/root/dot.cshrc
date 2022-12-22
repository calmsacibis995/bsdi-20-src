alias mail Mail
set history=1000
set path=(/sbin /usr/sbin /bin /usr/bin /usr/local/bin /usr/contrib/bin)
setenv BLOCKSIZE 1k

# directory stuff: cdpath/cd/back
set cdpath=(/sys /usr/src/{bin,sbin,usr.{bin,sbin},lib,libexec,share,contrib,local,games,old,})
alias	cd	'set old=$cwd; chdir \!*'
alias	h	history
alias	j	jobs -l
alias	ll	ls -lg
alias	back	'set back=$old; set old=$cwd; cd $back; unset back; dirs'

alias	z		suspend
alias	x	exit
alias	pd	pushd
alias	pd2	pushd +2
alias	pd3	pushd +3
alias	pd4	pushd +4
alias	tset	'set noglob histchars=""; eval `\tset -s \!*`; unset noglob histchars'

if ($?prompt) then
	if ( $prompt:q =~ *\#* ) then
		set prompt="`hostname -s`# "
	else
		set prompt="`hostname -s`% "
	endif
endif
