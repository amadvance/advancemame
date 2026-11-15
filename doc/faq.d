Name{number}
	faq - AdvanceMAME FAQ

	This is a collection of Frequently Asked Questions.

SubSubIndex

General FAQ
  Where can I get the latest version of this FAQ ?
	The latest version of the FAQ is included with the latest version
	of AdvanceMAME, available for download from the project homepage
	http://www.advancemame.it/.

	The latest version of the FAQ is also available online at
	http://www.advancemame.it/doc-faq.html.

  What is MAME ?
	MAME means Multiple Arcade Machine Emulator and is a huge effort
	to create an emulator capable of running a lot of old arcade
	games. The MAME homepage is at http://www.mame.net/.

  How is AdvanceMAME different from MAME ?
	AdvanceMAME is an unofficial port of MAME, written mainly by 
	Andrea Mazzoleni, with contributions from various other people. 
	The video support in AdvanceMAME is made especially to be able to 
	program the video hardware (graphics adapter) to achieve a more 
	faithful video emulation, also allowing the use with arcade 
	monitors and TVs.

	It also contains software filters to improve the image when used
	on monitors with a higher resolution, such as PC monitors and
	LCD screens.

  I have a question that is not answered in this FAQ. Where can I get help ?
	If you have looked through the documentation and still haven't 
	found an answer to your question, you could try posting it to the
	AdvanceMAME mailing list. Before you post, however, you should
	check the mailing list archives to make sure your question has not
	already been answered there. Subscription to the mailing list and
	archives can be found at
	http://lists.sourceforge.net/lists/listinfo/advancemame-users.

AdvanceMAME FAQ
  What MAME version is AdvanceMAME based on ?
	AdvanceMAME is based on the MAME 0.106 source code.

  How do I automatically record video and audio clips ?
	Add this script to your advmame.rc. For any game started a 15-second
	clip is recorded.

	:script_emulation delay(5000); \
	:	simulate_event(ui_snapshot, 50); \
	:	simulate_event(ui_record_start, 50); \
	:	delay(15100); \
	:	simulate_event(ui_record_stop, 50); \
	:	delay(100); \
	:	simulate_event(ui_cancel, 50);

  When I start a game with AdvanceMAME I see a copyright message before the
  game starts. How do I get rid of it ?
	Take a look at the `misc_quiet` option.

AdvanceMENU FAQ
  Each time I start AdvanceMENU, I see a message about
  "Updating the ... information file ..." How do I get rid of it ?
	The information file is recreated if the emulator executable
	is newer than the named file. If this happens all the time, you probably
	have a wrong time setting on your system.

  Can I use AdvanceMENU to list my MP3 and MNG collection ?
	Yes. You can create a fake emulator to play all your music
	and video clips. Add the following options in your
	advmenu.rc :

	:emulator "mng" generic ""
	:emulator_altss "mng" "C:\CLIP"
	:emulator_roms "mng" "C:\CLIP"
	:emulator_roms_filter "mng" "*.mng"

	:emulator "mp3" generic ""
	:emulator_altss "mp3" "C:\MP3;C:\MUSIC"
	:emulator_roms "mp3" "C:\MP3;C:\MUSIC"
	:emulator_roms_filter "mp3" "*.mp3"

  How do I avoid AdvanceMENU jumping to different games when I press
  letter keys ?
	Use `event_alpha no` in the AdvanceMENU configuration file. This
	is most often used if you have a control panel wired to a keyboard
	hack or keyboard controller.

DOS FAQ
  How can I solve the compiler error "Argument list too long (E2BIG)"
  and "Error -1" ?
	If compiling you get the "Argument list too long" error,
	or "Error -1" you need to use the DJGPP stubedit utility
	to increase the bufsize value for some DJGPP tools, with the
	following commands:

	:stubedit c:\djgpp\bin\gcc.exe bufsize=32k
	:stubedit c:\djgpp\bin\ld.exe bufsize=32k
	:stubedit c:\djgpp\bin\make.exe bufsize=32k
	:stubedit c:\djgpp\lib\gcc-lib\djgpp\3.23\collect2.exe bufsize=32k

	You may need to use different paths to the files,
	especially for collect2.exe.

  What's the best configuration in PURE DOS ?
	My favorite choice is to use HIMEM, EMM386 and a small
	cache with SMARTDRV.

	In CONFIG.SYS:
	:dos=high
	:device=C:\DOS\W98\HIMEM.SYS
	:device=C:\DOS\EMM386.EXE NOEMS NOVCPI

	In AUTOEXEC.BAT:
	:smartdrv 1024

	If you have a lot of RAM you can use a bigger disk cache like :
	:smartdrv 8192

  Why can I use only 32M of RAM ? I have more RAM.
	Start the EMM386.EXE device in your CONFIG.SYS with the
	"NOEMS NOVCPI" arguments.

  Why can I use only 64M of RAM ? I have more RAM.
	Don't use the DOS 6.22 version of the HIMEM.SYS device. Use a
	newer version, the Windows 98 version works well. You don't
	need to upgrade your DOS, simply copy the newer HIMEM.SYS
	from a Windows 98 installation.

  Which is the minimum memory requirement ?
	With 64 MB of memory you are able to play all the games.
	At least in PURE DOS with CWSDPMI 5 and with 512 MB of virtual
	memory.

  Which is the maximum memory required ?
	With 128 MB you should be able to play all the games without
	any big speed loss. For the bigger games you may need 512
	MB of virtual memory.

  How can I use two mice (or a trackball and a spinner) ?
	To get support for a second mouse you need the mouse driver
	in the contrib/optimous directory. This directory also contains
	a short text instruction file.

Linux FAQ
  Can I run the Advance programs from a readonly filesystem ?
	Yes. All the writable files are put in the $HOME/.advance
	directory. If they are readonly, they are not updated.

Windows FAQ
  How do I disable the Windows hotkeys ?
	Check the Microsoft online documents "Disabling the Windows Key
	on Microsoft Natural Keyboard" at http://support.microsoft.com/?kbid=181348.

Video FAQ
  How can I enable the TV-OUT signal (S-VIDEO) of the video board ?
	You cannot. If the TV-OUT signal is not enabled automatically
	by your video board you cannot enable it manually.

	If you have a TV with SCART input (all European TVs) you can buy
	or build a VGA to SCART cable.

  Can I connect my Arcade Monitor to the video board and use it with AdvanceMENU and
  AdvanceMAME ?
	Yes, this is one of the primary goals of the AdvanceMAME video output layer.
	But remember that arcade monitors can only accept a certain range of signals
	(in terms of horizontal and vertical refresh etc.), so make sure you 
	configure these settings correctly in AdvanceMENU and AdvanceMAME before
	you connect it to your monitor.

  How can I connect my Arcade Monitor to the video board ?
	If you do not want to use one of the available adaptors that ensure the 
	signal is within specifications, to avoid damaging your monitor, you can
	simply connect the monitor directly to the graphics card. If your monitor
	only has one sync-line (so-called composite sync) you will need to convert
	the separate sync signals of the video board to composite sync. This can be
	achieved by a circuit such as 
	http://www.tkk.fi/Misc/Electronics/circuits/vga2rgbs_ttl.gif

	Some video boards can be configured to generate the composite sync signal.
	An example is Matrox cards under Linux. Look at the matroxfb documentation
	in the kernel sources, and maybe the matroxset utility.

  How can I avoid damaging my arcade monitor due to the signals sent out during
  system startup (before AdvanceMENU and AdvanceMAME starts) ?
	There exist separate hardware adaptors that can be used for this
	purpose, such as the J-PAC http://www.ultimarc.com/jpac.html

	Other options include waiting until AdvanceMENU has started up, before
	connecting the monitor. This can be achieved either manually, or by 
	hardware controlled by the parallel port (as described in various places).

	If your system is running DOS, you can use the "vga" and "video" utilities
	of the AdvanceCAB package to set the graphics card in an acceptable mode
	while the system is booting. Note that the POST (Power On Self Test) screen
	of the BIOS will still be in the "bad" video-mode, so the problem is not 
	solved entirely by these utilities, but they allow for a nice bootup screen
	to be displayed.

	With some graphics adapters (e.g. Matrox), a similar effect can be achieved 
	under Linux by configuring the video mode of the kernel framebuffer.

  What is the "tearing" effect ?
	The tearing effect is a defect of the animation caused by a wrong
	synchronization between the game refresh rate and the video mode refresh
	rate. It generally appears as a horizontal image split line; on the
	top of this line you see the previous game frame, on the bottom of this
	line you see the next game frame.

	If the game and the video refresh rate are only a little different
	the tearing split line is slowly moving up or down. If the refreshes are
	very different the split line is moving randomly.

	The tearing effect is mostly noticeable in games with a continuously
	scrolling background.

  How can I remove the tearing effect ?
	The only way to remove the tearing effect is to enable the
	`display_vsync` option. To work correctly this option needs a video
	mode with a refresh rate which is a multiple of the original game
	refresh rate. For example for a 50 Hz game you can use video modes
	of 50, 100, 150 Hz.

  How can I reduce the tearing effect ?
	If for some reason you cannot use the `display_vsync` option the
	only way to reduce the tearing effect is to choose a video mode
	refresh rate which ensures a randomly and fast moving tearing
	split line.

	Generally these are high refresh rates, prime with the original game
	frequency.

  Does the "double/triple buffer" help to reduce the tearing effect ?
	No. A generic asynchronous buffer doesn't remove or reduce the
	tearing effect. A synchronous buffer instead is exactly like
	vsync. AdvanceMAME supports only asynchronous buffers.

  Can I get a good animation with a video mode with arbitrary frequency ?
	There is no way to get a perfect animation without using
	a video mode with the same game refresh rate. You will get the
	tearing effect or a not smooth animation from some frames skipped
	or displayed too long.

	For example, suppose you use a 70 Hz video mode with a 60 Hz game.
	You can display every frame for 1/60 of a second and get the tearing
	effect. Alternatively you can display every frame for 1/70 of a second,
	but after 7 frames you need to display a frame for 2/70 of a second to
	synchronize with the original game speed.

	The only exception is for frequencies which are multiples of
	the original game refresh rate.

  Why are the smaller video modes like 256x224 not working ?
	Your video board probably isn't able to generate the very low
	pixel clocks needed. You can try to increase the minimum value 
	of the `device_video_pclock` option. The video mode generation 
	will automatically generate video modes with double or triple
	horizontal resolutions to force the use of a pixel clock in
	the specified range.

License FAQ
  Under which license are AdvanceMAME and AdvanceMESS distributed ?
	Starting with version 6.0, AdvanceMAME and AdvanceMESS are
	distributed under the GNU General Public License version 2
	or later (GPL-2.0+).

	Individual source files retain their original GPL-compatible
	licenses (such as BSD-3-Clause, LGPL-2.1+, MIT, Zlib, or Public
	Domain) and copyright attributions, as indicated in their
	headers.

  Is MAME 0.106 released under the GPL ?
	No. MAME 0.106 was originally released under the old
	non-commercial MAME license. MAME 0.172 was the first official
	release under the new open-source licensing scheme.

  How can AdvanceMAME be GPL-licensed if it is based on MAME 0.106 ?
	The relicensing is not based on the MAME 0.172 release itself.
	During the MAME relicensing effort, the individual copyright
	holders gave permission to distribute their contributions to
	MAME under standard open-source licenses, such as BSD-3-Clause,
	LGPL-2.1+ or GPL-2.0+. These permissions apply to the
	contributions themselves and are not bound to a specific MAME
	release.

	AdvanceMAME applies these permissions file by file. For each
	source file, the authors of the code it contains were checked
	against the official MAME relicensing records, and the
	resulting license and copyright holders are stated in the file
	header. The audit is documented in `support/relicense/`.

	Code from authors who declined or could not be reached was
	removed or replaced with clean, GPL-compatible rewrites
	backported from later MAME versions.

  What if I am a copyright holder and find a licensing or attribution issue ?
	The license audit and code replacements were carried out in good
	faith to ensure full GPL-2.0+ compliance. If you are a copyright
	holder and believe any portion of your code was not properly
	relicensed or attributed, please contact the maintainer or open
	an issue on the project repository so that it can be promptly
	corrected, removed, or replaced.

  Are game ROMs and software images covered by the GPL ?
	No. Game ROMs, firmware and software images are separate
	copyrighted works. Their use and redistribution depend on
	the permissions of their respective copyright holders.

Copyright
	This file is Copyright (C) 2003, 2004, 2005, Andrea Mazzoleni,
	Mikkel Holm Olsen (http://symlink.dk/)
