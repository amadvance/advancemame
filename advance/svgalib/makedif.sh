src=../../../svgalib

diff -u -b $src . | grep -v "^Only" | grep -v "^Common" | grep -v "^Binary" > svgalib.dif
diff -u -b $src/drivers drivers | grep -v "^Only" | grep -v "^Common" | grep -v "^Binary" >> svgalib.dif
diff -u -b $src/ramdac ramdac | grep -v "^Only" | grep -v "^Common" | grep -v "^Binary" >> svgalib.dif
diff -u -b $src/clockchi clockchi | grep -v "^Only" | grep -v "^Common" | grep -v "^Binary" >> svgalib.dif

