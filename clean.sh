rm -f p2p.db
rm -f trackerinfo.txt
(cd blockchain; make clean)
(cd p2p; make clean)
(cd p2p/sqlite3; make clean)
