#!/bin/bash
##
sudo cp DpssSeq512.txt dpssSeq.txt

#sudo ./ran_build/build/nr-uesoftmodem -r 24 --numerology 1 -C 3604320000 -s 24 --sa --ue-fo-compensation --nokrnmod 1 -O ~/openairinterface_w25/openairinterface5g/targets/PROJECTS/GENERIC-NR-5GC/CONF/ue_oai.conf --usrp-args "addr=192.168.20.2,clock_source=internal,time_source=internal" --ue-txgain 35 --ue-rxgain 40


sudo ./ran_build/build/nr-uesoftmodem -r 24 --numerology 1 --band 78 -C 3604320000 -s 24 --sa --ue-fo-compensation --nokrnmod 1 -O ~/openairinterface_w25/openairinterface5g/targets/PROJECTS/GENERIC-NR-5GC/CONF/ue_oai.conf --usrp-args "addr=192.168.20.2,clock_source=internal,time_source=internal" --dlsch-parallel 8  --ue-txgain 35 --ue-rxgain 40

#sudo ./ran_build/build/nr-uesoftmodem -r 24 --numerology 1 -C 3604365500 -s 24 --sa --nokrnmod 1 -O ~/openairinterface_w25/openairinterface5g/targets/PROJECTS/GENERIC-NR-5GC/CONF/ue_oai.conf --usrp-args "addr=192.168.20.2,clock_source=internal,time_source=internal" --ue-txgain 35 --ue-rxgain 60

