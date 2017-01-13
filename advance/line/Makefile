ECHO=echo
LINE=./advline

TARGET = advline pcvga.rc pcsvga60.rc standard.rc medium.rc extended.rc pal.rc ntsc.rc \
	vgamode.dat svgamode.dat raspberry.rc raspberry.dat

all: $(TARGET)

$(LINE): line.cc
	c++ -g -Wall -O0 line.cc -I ../common -o $@

vgamode.dat: $(LINE)
	echo "/* Video modes computed with the advline utility */" > $@
	$(LINE) /c_format /show_vga /show_comment /show_header /vga >> $@
	$(LINE) /c_format /show_vga /show_comment /show_header /pal >> $@
	$(LINE) /c_format /show_vga /show_comment /show_header /ntsc >> $@
	$(LINE) /c_format /show_vga /show_comment /show_header /atari_standard /v 60 >> $@
	$(LINE) /c_format /show_vga /show_comment /show_header /atari_extended /v 53 >> $@
	$(LINE) /c_format /show_vga /show_comment /show_header /atari_medium /v 60 >> $@

svgamode.dat: $(LINE)
	echo "/* Video modes computed with the advline utility */" > $@
	$(LINE) /c_format /show_svga /show_comment /show_header /vga >> $@
	$(LINE) /c_format /show_svga /show_comment /show_header /pal >> $@
	$(LINE) /c_format /show_svga /show_comment /show_header /ntsc >> $@
	$(LINE) /c_format /show_svga /show_comment /show_header /atari_standard >> $@
	$(LINE) /c_format /show_svga /show_comment /show_header /atari_extended >> $@
	$(LINE) /c_format /show_svga /show_comment /show_header /atari_medium >> $@
	$(LINE) /c_format /show_svga /show_comment /show_header /svga60 >> $@
	$(LINE) /c_format /show_svga /show_comment /show_header /svga57 >> $@

raspberry.dat: $(LINE)
	echo "/* Video modes computed with the advline utility */" > $@
	$(LINE) /c_format /sync_pp /no_d /pal -n raspberry_DPI_pal /show_fit /p 4.8 >> $@
	$(LINE) /c_format /sync_pp /no_d /pal -n raspberry_DPI_pal /show_fit /p 6.4 >> $@
	$(LINE) /c_format /sync_pp /no_d /pal -n raspberry_DPI_pal /show_fit /p 9.6 >> $@
	$(LINE) /c_format /sync_pp /no_d /pal -n raspberry_DPI_pal /show_fit /p 19.2 >> $@
	$(LINE) /c_format /sync_pp /no_d /ntsc -n raspberry_DPI_ntsc /show_fit /p 4.8 >> $@
	$(LINE) /c_format /sync_pp /no_d /ntsc -n raspberry_DPI_ntsc /show_fit /p 6.4 >> $@
	$(LINE) /c_format /sync_pp /no_d /ntsc -n raspberry_DPI_ntsc /show_fit /p 9.6 >> $@
	$(LINE) /c_format /sync_pp /no_d /ntsc -n raspberry_DPI_ntsc /show_fit /p 19.2 >> $@
	$(LINE) /c_format /sync_pp /no_d /atari_standard -n raspberry_DPI_standard /show_fit /p 4.8 >> $@
	$(LINE) /c_format /sync_pp /no_d /atari_standard -n raspberry_DPI_standard /show_fit /p 6.4 >> $@
	$(LINE) /c_format /sync_pp /no_d /atari_standard -n raspberry_DPI_standard /show_fit /p 9.6 >> $@
	$(LINE) /c_format /sync_pp /no_d /atari_standard -n raspberry_DPI_standard /show_fit /p 19.2 >> $@
	$(LINE) /c_format /sync_pp /no_d /atari_extended -n raspberry_DPI_extended /show_fit /p 4.8 >> $@
	$(LINE) /c_format /sync_pp /no_d /atari_extended -n raspberry_DPI_extended /show_fit /p 6.4 >> $@
	$(LINE) /c_format /sync_pp /no_d /atari_extended -n raspberry_DPI_extended /show_fit /p 9.6 >> $@
	$(LINE) /c_format /sync_pp /no_d /atari_extended -n raspberry_DPI_extended /show_fit /p 19.2 >> $@
	$(LINE) /c_format /sync_pp /no_d /atari_medium -n raspberry_DPI_medium /show_fit /p 4.8 >> $@
	$(LINE) /c_format /sync_pp /no_d /atari_medium -n raspberry_DPI_medium /show_fit /p 6.4 >> $@
	$(LINE) /c_format /sync_pp /no_d /atari_medium -n raspberry_DPI_medium /show_fit /p 9.6 >> $@
	$(LINE) /c_format /sync_pp /no_d /atari_medium -n raspberry_DPI_medium /show_fit /p 19.2 >> $@

pcvga.rc: $(LINE)
	$(ECHO) $@
	$(ECHO) "# Example configuration file for PC VGA Monitors" > $@
	$(ECHO) >> $@
	$(ECHO) "device_video_pclock 10-50" >> $@
	$(ECHO) "device_video_hclock 31.5" >> $@
	$(ECHO) "device_video_vclock 55-100" >> $@
	$(ECHO) >> $@
	$(LINE) /show_comment /show_header /show_vga /show_svga /vga >> $@

pcsvga60.rc: $(LINE)
	$(ECHO) $@
	$(ECHO) "# Example configuration file for PC Multisync Monitors" > $@
	$(ECHO) >> $@
	$(ECHO) "device_video_pclock 10-80" >> $@
	$(ECHO) "device_video_hclock 30-60" >> $@
	$(ECHO) "device_video_vclock 55-100" >> $@
	$(ECHO) >> $@
	$(LINE) /show_comment /show_header /show_vga /show_svga /svga60 >> $@

standard.rc: $(LINE)
	$(ECHO) $@
	$(ECHO) "# Example configuration file for Arcade Standard Resolution (15 kHz) Monitors (CGA)" > $@
	$(ECHO) >> $@
	$(ECHO) "device_video_pclock 5-50" >> $@
	$(ECHO) "device_video_hclock 15.75" >> $@
	$(ECHO) "device_video_vclock 50-60" >> $@
	$(ECHO) >> $@
	$(LINE) /atari_standard /show_comment /show_header >> $@
	$(LINE) /atari_standard /show_comment /v 60 /show_vga >> $@
	$(LINE) /atari_standard /show_comment /show_svga >> $@

medium.rc: $(LINE)
	$(ECHO) $@
	$(ECHO) "# Example configuration file for Arcade Medium Resolution (25 kHz) Monitors (EGA)" > $@
	$(ECHO) >> $@
	$(ECHO) "device_video_pclock 5-50" >> $@
	$(ECHO) "device_video_hclock 25" >> $@
	$(ECHO) "device_video_vclock 50-60" >> $@
	$(ECHO) >> $@
	$(LINE) /atari_medium /show_comment /show_header >> $@
	$(LINE) /atari_medium /show_comment /v 60 /show_vga >> $@
	$(LINE) /atari_medium /show_comment /show_svga >> $@

extended.rc: $(LINE)
	$(ECHO) $@
	$(ECHO) "# Example configuration file for Arcade Extended Resolution (16.5 kHz) Monitors" > $@
	$(ECHO) >> $@
	$(ECHO) "device_video_pclock 5-50" >> $@
	$(ECHO) "device_video_hclock 16.5" >> $@
	$(ECHO) "device_video_vclock 50-60" >> $@
	$(ECHO) >> $@
	$(LINE) /atari_extended /show_comment /show_header >> $@
	$(LINE) /atari_extended /show_comment /v 53 /show_vga >> $@
	$(LINE) /atari_extended /show_comment /show_svga >> $@

pal.rc: $(LINE)
	$(ECHO) $@
	$(ECHO) "# Example configuration file for PAL TVs" > $@
	$(ECHO) >> $@
	$(ECHO) "device_video_pclock 5-50" >> $@
	$(ECHO) "device_video_hclock 15.62" >> $@
	$(ECHO) "device_video_vclock 50" >> $@
	$(ECHO) >> $@
	$(LINE) /show_comment /show_header /show_vga /show_svga /pal >> $@

ntsc.rc: $(LINE)
	$(ECHO) $@
	$(ECHO) "# Example configuration file for NTSC TVs" > $@
	$(ECHO) >> $@
	$(ECHO) "device_video_pclock 5-50" >> $@
	$(ECHO) "device_video_hclock 15.73" >> $@
	$(ECHO) "device_video_vclock 60" >> $@
	$(ECHO) >> $@
	$(LINE) /show_comment /show_header /show_vga /show_svga /ntsc >> $@

raspberry.rc: $(LINE)
	$(ECHO) $@
	$(ECHO) "# PAL TVs (15.625 kHz)" >> $@
	$(LINE) /sync_nn /no_d /pal -n raspberry_DPI_pal /show_fit /p 4.8 >> $@
	$(LINE) /sync_nn /no_d /pal -n raspberry_DPI_pal /show_fit /p 6.4 >> $@
	$(LINE) /sync_nn /no_d /pal -n raspberry_DPI_pal /show_fit /p 9.6 >> $@
	$(LINE) /sync_nn /no_d /pal -n raspberry_DPI_pal /show_fit /p 19.2 >> $@
	$(ECHO) >> $@
	$(ECHO) "# NTSC TVs (15.734 kHz)" >> $@
	$(LINE) /no_d /ntsc -n raspberry_DPI_ntsc /show_fit /p 4.8 >> $@
	$(LINE) /no_d /ntsc -n raspberry_DPI_ntsc /show_fit /p 6.4 >> $@
	$(LINE) /no_d /ntsc -n raspberry_DPI_ntsc /show_fit /p 9.6 >> $@
	$(LINE) /no_d /ntsc -n raspberry_DPI_ntsc /show_fit /p 19.2 >> $@
	$(ECHO) >> $@
	$(ECHO) "# Arcade Standard Resolution (15.72 kHz) Monitors (CGA)" >> $@
	$(LINE) /no_d /atari_standard -n raspberry_DPI_standard /show_fit /p 4.8 >> $@
	$(LINE) /no_d /atari_standard -n raspberry_DPI_standard /show_fit /p 6.4 >> $@
	$(LINE) /no_d /atari_standard -n raspberry_DPI_standard /show_fit /p 9.6 >> $@
	$(LINE) /no_d /atari_standard -n raspberry_DPI_standard /show_fit /p 19.2 >> $@
	$(ECHO) >> $@
	$(ECHO) "# Arcade Extended Resolution (16.5 kHz) Monitors" >> $@
	$(LINE) /no_d /atari_extended -n raspberry_DPI_extended /show_fit /p 4.8 >> $@
	$(LINE) /no_d /atari_extended -n raspberry_DPI_extended /show_fit /p 6.4 >> $@
	$(LINE) /no_d /atari_extended -n raspberry_DPI_extended /show_fit /p 9.6 >> $@
	$(LINE) /no_d /atari_extended -n raspberry_DPI_extended /show_fit /p 19.2 >> $@
	$(ECHO) >> $@
	$(ECHO) "# Arcade Medium Resolution (25 kHz) Monitors" >> $@
	$(LINE) /no_d /atari_medium -n raspberry_DPI_medium /show_fit /p 4.8 >> $@
	$(LINE) /no_d /atari_medium -n raspberry_DPI_medium /show_fit /p 6.4 >> $@
	$(LINE) /no_d /atari_medium -n raspberry_DPI_medium /show_fit /p 9.6 >> $@
	$(LINE) /no_d /atari_medium -n raspberry_DPI_medium /show_fit /p 19.2 >> $@

clean:
	rm -f $(TARGET)

