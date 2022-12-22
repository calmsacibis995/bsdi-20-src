PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/local/bin:/usr/contrib/bin
echo 'erase ^H, kill ^U, intr ^C status ^T'
stty crt erase  kill  intr  status 
export PATH
umask 022
HOME=/root
export HOME
BLOCKSIZE=1k
export BLOCKSIZE
TERM=ibmpc3
export TERM
