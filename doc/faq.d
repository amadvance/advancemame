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
	AdvanceMAME is a port of MAME, written mainly by Andrea Mazzoleni, 
	with contributions from various other people. The video support in
	AdvanceMAME is made especially to be able to program the video
	hardware (graphics adapter) to achieve a more faithful video 
	emulation, also allowing the use with arcade monitors and TVs.

	It also contains software filters to improve the image when used
	on monitors with a higher resolution, such as PC monitors and
	LCD screens

  I have a question that is not answered in this FAQ. Where can I get help ?
	If you have looked through the documentation and still haven't 
	found an answer to your question, you could try posting it to the
	AdvanceMAME mailing list. Before you post, however, you should
	check the mailing list archives to make sure your question has not
	already been answered there. Subscription to the mailing list and
	archives can be found at
	http://lists.sourceforge.net/lists/listinfo/advancemame-users.

AdvanceMAME FAQ
  What MAME version AdvanceMAME is based on ?
	AdvanceMAME is based on the MAME 0.106 source code.

  How do I automatically record video and audio clips ?
	Add this script to your advmame.rc. For any game started a 15 seconds
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
	Take a look at the `misc_quiet' option.

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
	Use `event_alpha no' in the AdvanceMENU configuration file. This
	is most often used if you have a control panel wired to a keyboard
	hack or keyboard controller.

DOS FAQ
  How can I solve the compiler error "Argument list too long (E2BIG)"
  and "Error -1" ?
	If compiling you get the "Argument list too long" error,
	or "Error -1" you need to use the DJGPP stubedit utility
	to increase bufsize value for some DJGPP tools, with the
	following commands:

	:stubedit c:\djgpp\bin\gcc.exe bufsize=32k
	:stubedit c:\djgpp\bin\ld.exe bufsize=32k
	:stubedit c:\djgpp\bin\make.exe bufsize=32k
	:stubedit c:\djgpp\lib\gcc-lib\djgpp\3.23\collect2.exe bufsize=32k

	You may need to use different paths to the files,
	especially for collect2.exe.

  What's the best configuration in PURE DOS ?
	My favorite choice is to use HIMEM, EMM386 and a little
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
	newer version, the Windows 98 version works good. You don't
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

	If you have a TV with SCART input (all european TVs) you can buy
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
	An example is Matrox cards under Linux. Look at the matroxfb-documentation
	in the kernel sources, and maybe the matroxset-utility.

  How can I avoid damaging my arcade monitor due to the signals sent out during
  system startup (before AdvanceMENU and AdvanceMAME starts) ?
	There exists separate hardware adaptors that can be used for this
	purpose, such as the J-PAC http://www.ultimarc.com/jpac.html

	Other options include waiting until AdvanceMENU has started up, before
	connecting the monitor. This can be achieved either manually, or by 
	hardware controlled by the parallel port (as described in various places).

	If your system is running DOS, you can use the "vga" and "video" utilities
	of the AdvanceCAB package to set the graphics card in an acceptable mode
	while the system is booting. Note that the POST (Power On Self Test) screen
	of the BIOS will still be in the "bad" video-mode, so the problem is not 
	solved entirely by these utilities, but they allow for a nice bootup-screen
	to be displayed.

	With some graphics adapters (eg. Matrox), a similar effect can be achieved 
	under Linux by configuring the video mode of the kernel framebuffer.

  What is the "tearing" effect ?
	The tearing effect is a defect of the animation caused by a wrong
	synchronization with the game refresh rate and the video mode refresh
	rate. It generally appears as a horizontal image split line, on the
	top of this line you see the previous game frame, on the bottom of this
	line you see the next game frame.

	If the game and the video refresh rate are only a little different
	the tearing split line is slowly moving up or down. If the refreshes are
	very different the split line is moving randomly.

	The tearing effect is mostly noticeable in games with a continuously
	scrolling background.

  How can I remove the tearing effect ?
	The only way to remove the tearing effect is to enable the
	`display_vsync' option. To correctly work this option need a video
	mode with a refresh rate which is a multiplier of the original game
	refresh rate. For example for a 50 Hz game you can use video modes
	of 50, 100, 150 Hz.

  How can I reduce the tearing effect ?
	If for some reason you cannot use the `display_vsync' option the
	only way to reduce the tearing effect is to choose a video mode
	refresh rate which ensure to have a randomly and fast moving tearing
	split line.

	Generally they are high refresh rates, prime with the original game
	frequency.

  Does the "double/tripler buffer" help to reduce the tearing effect ?
	No. A generic asynchronous buffer doesn't remove or reduce the
	tearing effect. A synchronous buffer instead it's exactly like
	the vsync. AdvanceMAME supports only the asynchronous buffers.

  Can I get a good animation with a video mode with arbitrary frequency ?
	There is no way to get a perfect animation without using
	a video mode with the same game refresh rate. You will get the
	tearing effect or a not smooth animation from some frames skipped
	or displayed too long.

	For example, suppose to use a 70 Hz video mode with a 60 Hz game.
	You can display every frame for 1/60 of second and get the tearing
	effect. Alternatively you can display every frame for 1/70 of second,
	but after 7 frame you need to display a frame for 2/70 of second to
	synchronize with the original game speed.

	The only exception is for frequencies which are multiplier of
	the original game refresh rate.

  Why are the smaller video modes like 256x224 not working ?
	Your video board probably isn't able to generate the very low
	pixelclocks needed. You can try to increase the minimum value 
	of the `device_video_pclock' option. The video mode generation 
	will automatically generate video modes with double or triple
	horizontal resolutions to force the use of a pixelclock in
	the specified range.

License FAQ
  Is MAME 0.106 Open Source/Free Software ?
	No. The MAME 0.106 License is not an Open Source License as defined
	at http://www.opensource.org/ and not a Free Software License as defined
	at http://www.gnu.org/.

  How can be MAME 0.106 not Open Source/Free Software when the latest
  MAME is released with the GPL 2 License ?
	MAME 0.106 was never released with the GPL 2 License.
	Most, if not all, same source code can be found released
	with the GPL 2 in the latest MAME, but it's safer to
	assume that MAME 0.106 is not GPL 2.

  Is AdvanceMAME Open Source and Free Software ?
	Yes. AdvanceMAME is released with the GPL License which is
	an Open Source and a Free Software License.

  Which is the license of the AdvanceMAME source ?
	The AdvanceMAME project is released under the GPL license.

	In addition you have some extra rights granted by a special
	license exception which allow you to link the AdvanceMAME GPL
	source with the MAME 0.106 source.

	The exception also gives you the rights to eliminate it
	if you don't like it or if you want to include the
	AdvanceMAME source in another GPL program.

	You can more easily think at it as a sort of double license.
	A GPL or a GPL + exception.

  Why the AdvanceMAME license is called GPL if it's different
  than the GPL ?
	The GPL license used by AdvanceMAME isn't modified in any
	way.

	The license exception is added outside the GPL license text.
	It's added at the top of every source files for which the
	exception has the validity.

	How to add this sort of exceptions is specifically covered on
	the Official GPL FAQ written by the same people which have
	written the GPL license and which are the copyright holders
	of the GPL license.

	Specifically, in the Official GPL FAQ you can see that in
	every source files with the license exception you must add
	the following text :

	"This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version."

	Which explicitly claims that the program with the license
	exception is still GPL.

  Does the MAME 0.106 license allow linking the MAME 0.106 source with
  a GPL source ?
	Yes. The only requirement of the MAME 0.106 License for the
	derivative works is the whole source availability.
	With the GPL you have it.

  Does the GPL license allow to link a GPL source with a
  MAME 0.106 source ?
	No. The GPL license requires that the whole program be
	released with the GPL license or with a GPL compatible
	license. The MAME 0.106 License isn't a GPL compatible license.

  Does the GPL license + exception allow linking a
  GPL + exception source with a MAME 0.106 source ?
	Yes. The license exception specifically allows it.

  The license exception say "MAME library". Is MAME a library or
  a program ?
	AdvanceMAME uses only the internal MAME 0.106 core, which is
	effectively a library.

	Anyway, calling a piece of software a program or a library 
	doesn't change its license.

  The GPL FAQ say: "Combining two modules means connecting
  them together so that they form a single larger program. If
  either part is covered by the GPL, the whole combination must
  also be released under the GPL--if you can't, or won't, do
  that, you may not combine them.". Is it true ?
	Yes. But the GPL FAQ speak of a generic GPL program without
	any exception.

	The license exception explicitly allows you to combine the
	GPL program with MAME 0.106. These exceptions exist specifically
	to solve this problem. Essentially, if your GPL program needs
	to use another module which is not under a GPL license, you 
	(as copyright holder) can grant at the user the rights to link 
	it with the not GPL module.

	The GPL was made to protect the interests of the copyright
	holder. But if the copyright holder wants, he can relax the
	license adding specific exceptions that extend the rights of
	the users. So, the users can do things not generally
	permitted by the standard GPL license.

  What license is the AdvanceMAME binary under ?
	The AdvanceMAME binary contains source from AdvanceMAME,
	MAME, Allegro, SEAL, svgalib, zlib and FreeType2. You must
	follow ALL these licenses when you use the binary.
	So, you are only allowed to do things with it, if they are
	permitted by ALL these licenses.

  Can I sell the AdvanceMAME binary ?
	No. The AdvanceMAME binary contains compiled MAME 0.106 source and
	the "MAME 0.106 License" prohibits selling.

  Who is the copyright holder of AdvanceMAME ?
	Andrea Mazzoleni is the copyright holder of AdvanceMAME.

	Some source and documentation files are also under the
	copyright of Filipe Estima, Ian Patterson, Randy Schnedler,
	S. Sakamaki, Mikkel Holm Olsen.

GPL FAQ
	This is an extract of the Official GPL FAQ present at :

		+http://www.gnu.org/licenses/gpl-faq.html

  I am writing free software that uses non-free libraries.
  What legal issues come up if I use the GPL?

	If the libraries that you link with falls within the
	following exception in the GPL:

		:However, as a special exception, the source code distributed need not
		:include anything that is normally distributed (in either source or
		:binary form) with the major components (compiler, kernel, and so on) of
		:the operating system on which the executable runs, unless that
		:component itself accompanies the executable.

	then you don't have to do anything special to use them. In
	other words, if the libraries you need come with major parts
	of a proprietary operating system, the GPL says people can
	link your program with them.

	If you want your program to link against a library not
	covered by that exception, you need to add your own
	exception, wholly outside of the GPL. This copyright notice
	and license notice give permission to link with the program
	FOO:

		:Copyright (C) yyyy

		:This program is free software; you can redistribute it and/or modify
		:it under the terms of the GNU General Public License as published by
		:the Free Software Foundation; either version 2 of the License, or
		:(at your option) any later version.

		:This program is distributed in the hope that it will be useful,
		:but WITHOUT ANY WARRANTY; without even the implied warranty of
		:MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
		:GNU General Public License for more details.

		:You should have received a copy of the GNU General Public License
		:along with this program; if not, write to the Free Software
		:Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

		:In addition, as a special exception, <name of copyright
		:holder> gives permission to link the code of this program with
		:the FOO library (or with modified versions of FOO that use the
		:same license as FOO), and distribute linked combinations including
		:the two.  You must obey the GNU General Public License in all
		:respects for all of the code used other than FOO.  If you modify
		:this file, you may extend this exception to your version of the
		:file, but you are not obligated to do so.  If you do not wish to
		:do so, delete this exception statement from your version.

	Only the copyright holders for the program can legally
	authorize this exception. If you wrote the whole program
	yourself, then assuming your employer or school does not
	claim the copyright, you are the copyright holder--so
	you can authorize the exception. But if you want to use
	parts of other GPL-covered programs by other authors in your
	code, you cannot authorize the exception for them. You have
	to get the approval of the copyright holders of those programs.

	When other people modify the program, they do not have to
	make the same exception for their code--it is their choice
	whether to do so.

	Adding this exception eliminates the legal issue, ...

	+http://www.gnu.org/licenses/gpl-faq.html#WritingFSWithNFLibs

  Is the developer of a GPL-covered program bound by the GPL? Could the
  developer's actions ever be a violation of the GPL?
  
	Strictly speaking, the GPL is a license from the developer for others
	to use, distribute and change the program. The developer itself is
	not bound by it, so no matter what the developer does, this is not a
	"violation" of the GPL.

	However, if the developer does something that would violate the GPL
	if done by someone else, the developer will surely lose moral
	standing in the community.

	+http://www.gnu.org/licenses/gpl-faq.html#TOCDeveloperViolate

  I want to distribute binaries, but distributing complete source is inconvenient.
  Is it ok if I give users the diffs from the "standard" version along with the
  binaries?

	This is a well-meaning request, but this method of providing the source
	doesn't really do the job.

	A user that wants the source a year from now may be unable to get the proper
	version from another site at that time. The standard distribution site may have
	a newer version, but the same diffs probably won't work with that version.

	So you need to provide complete sources, not just diffs, with the binaries.

	+http://www.gnu.org/licenses/gpl-faq.html#TOCDistributingSourceIsInconvenient

Copyright
	This file is Copyright (C) 2003, 2004, 2005, Andrea Mazzoleni,
	Mikkel Holm Olsen (http://symlink.dk/)

