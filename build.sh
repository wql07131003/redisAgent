set -e
make
mkdir -p output/{bin,conf,lib,log}
cp control/supervise output/
cp control/proxy_control output/bin/
cp control/control.conf output/conf/
touch output/{lib,log}/.placeholder
