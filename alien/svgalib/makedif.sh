src=../svgalib.ori

diff -u $src . | grep -v "^Only" | grep -v "^Common" | grep -v "^Binary" > svgalib.dif
diff -u $src/drivers drivers | grep -v "^Only" | grep -v "^Common" | grep -v "^Binary" >> svgalib.dif
diff -u $src/ramdac ramdac | grep -v "^Only" | grep -v "^Common" | grep -v "^Binary" >> svgalib.dif
diff -u $src/clockchi clockchi | grep -v "^Only" | grep -v "^Common" | grep -v "^Binary" >> svgalib.dif

