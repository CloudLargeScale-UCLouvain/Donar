#!/bin/bash

cd /usr/local/bin/
for bin in donar measlat tor2 tor3 torecho udpecho; do
  wget https://gitlab.inria.fr/qdufour/donar/-/raw/master/dist/${bin}?inline=false -O ${bin}
  chmod +x ${bin}
done

for scr in donaralt donaraltna donaraltopt \
           donardup donardupna donardupopt \
           torfone torfonena torfoneopt \
           torhs torhsna torhsopt \
           torexitna torexit; do
  wget https://gitlab.inria.fr/qdufour/donar/-/raw/master/scripts/2021/${scr}?inline=false -O ${scr}
  chmod +x ${scr}
done

cd /etc/systemd/system/
for svc in donaraltna@.service donaraltopt@.service donaralt@.service \
           donardupna@.service donardupopt@.service donardup@.service \
           torfonena@.service torfoneopt@.service torfone@.service \
           torhsna@.service torhsopt@.service torhs@.service \
           torexitna@.service torexitnahelper.service \
           torexit@.service torexithelper.service; do
  wget https://gitlab.inria.fr/qdufour/donar/-/raw/master/scripts/2021/${svc}?inline=false -O ${svc}
done

cd /root
for out in donaralt donaraltna donaraltopt \
           donardup donardupna donardupopt \
           torfone torfonena torfoneopt \
           torhs torhsna torhsopt \
           torexitna torexit; do
  mkdir ${out}
done
