Name
	faq - The Advance FAQ

	This is a collection of Frequently Asked Questions.

	The FAQ is organized in the following chapters :

		* AdvanceMAME FAQ
		* AdvanceMENU FAQ
		* DOS CONFIGURATION FAQ
		* LINUX CONFIGURATION FAQ
		* VIDEO CONFIGURATION FAQ
		* LICENSE FAQ

AdvanceMAME FAQ
	<empty>

AdvanceMENU FAQ
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

DOS Configuration FAQ
  What's the best configuration in PURE DOS ?
	My favourite choice is to use HIMEM, EMM386 and a little
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

  How can I use two mouse ?
	To use the second mouse support you need the mouse driver
	in the contrib/optimous directory. It's also present
	a short text instruction file.

Linux Configuration FAQ
  How can I load the svgalib_helper.o kernel module automatically ?
	Insert this line :

		:alias char-major-209 svgalib_helper

	in your /etc/modules.conf file and run only one time :

		:depmod -a

	After that the module is loaded automatically every time is
	needed.

  On scrolling games I see a missing frame every 5/10 seconds, also
  with vsync enabled. How can I fix it ?
	Try disabling the joystick use with the `device_joystick none'
	option, or upgrade at the svgalib 1.9.16 library which supports
	for some boards the vsync IRQ.

  How can I enable the XFree86 DGA extension ?
	Ensure that in your XF86Config file the line :

		:Option    "omit xfree86-dga"

	is not present or commented. To test it you can use
	the `dga' utility. Run :

		:man dga

Video Configuration FAQ
  Which is the "tearing" effect ?
	The tearing effect is a defects of the animation caused by a wrong
	syncronization with the game refresh rate and the video mode refresh
	rate. It generally appears as a horizontal image split line, on the
	top of this line you see the previous game frame, on the bottom of this
	line you see the next game frame.

	If the game and the video refresh rate are only a little different
	the tearing split line is slowly moving up or down. If the refreshs are
	very different the split line is moving randomily.

	The tearing effect is mostly noticeable in games with a continously
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
	No. A generic asyncronous buffer does't remove or reduce the
	tearing effect. A syncronous buffer instead it's exactly like
	the vsync. AdvanceMAME supports only the asyncronous buffers.

  Can I get a good animation with a video mode with arbitrary frequency ?
	There is no way to get a perfect animation without using
	a video mode with the same game refresh rate. You will get the
	tearing effect or a not smooth animation from some frames skipped
	or displayed too long.

	For example, suppose to use a 70 Hz video mode with a 60 Hz game.
	You can display every frame for 1/60 of second and get the tearing
	effect. Alternatively you can display every frame for 1/70 of second,
	but after 7 frame you need to display a frame for 2/70 of second to
	resyncronize with the original game speed.

	The only exception is for frequencies which are multipler of
	the original game refresh rate.

License FAQ
  Why SourceForge accepted the AdvanceMAME project and not the
  xmame project ?
	SourceForge accepts only Open Source licenses.

	The source distribution of AdvanceMAME is released with
	an Open Source license. So, it was accepted.

	The xmame source distribution contains the whole MAME
	sources and it is released with the "MAME License".
	The MAME License isn't an Open Source license as defined
	on http://www.opensource.org. So, it was rejected.

  AdvanceMAME contains MAME ?
	No. The AdvanceMAME source distribution doesn't contain
	the MAME sources.

	Anyway, to generate the AdvanceMAME binary you need the MAME
	sources. And you also need the Allegro, SEAL and zlib
	sources.

  Which is the license of the AdvanceMAME source ?
	The AdvanceMAME project is released under the GPL license.

	In addition you have some extra rights granted by a special
	license exception which allow you to link the AdvanceMAME GPL
	source with the not GPL MAME source.

	The exception give also to you the rights to eliminate it
	if you don't like it or if you want to include the
	AdvanceMAME source in another GPL program. So, AdvanceMAME
	is 100% GPL.

	You can more easily think at it as a sort of double license.
	A GPL or a GPL + exception. You have all the rights of the
	GPL, and, if you want, plus some others.

  Why the AdvanceMAME license is called GPL if it's different
  than the GPL ?
	The GPL license used by AdvanceMAME isn't modified in any
	way.

	The license exception is added outside the GPL license text.
	It's added at the top of every source files for which the
	exception has the validity.

	How to add this sort of exceptions is specifically covered on
	the OFFICIAL GPL FAQ written by the same people which have
	written the GPL license and which are the copyright holders
	of the GPL license.

	Specifically, in the OFFICIAL GPL FAQ you can see that in
	every source files with the license exception you must add
	the following text :

	"This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version."

	Which explicitly claims that the program with the license
	exception is still GPL.

  Does the MAME license allow linking the MAME source with
  a GPL source ?
	Yes. The only requirement of the MAME License for the
	derivative works is the whole source availability.
	With the GPL you have it.
	Obviously this assuming that the MAME License isn't changed
	and keeps its complete validity.

  Does the GPL license allow to link a GPL source with a
  MAME source ?
	No. The GPL license requires that the whole program be
	released with the GPL license or with a GPL compatible
	license. The MAME License isn't a GPL compatible license.

  Does the GPL license + exception allow linking a
  GPL + exception source with a MAME source ?
	Yes. The license exception specifically allows it.

  The license exception say "MAME library". Is MAME a library or
  a program ?
	AdvanceMAME uses only the internal MAME core, which is
	effectively a library.  For example MESS is distributed with
	the MAME core as a DLL (Dynamic Linked Library) and two
	different frontends as .EXE files.

	Anyway, calling a software program or library doesn't change
	his license.

  The GPL FAQ say: "Combining two modules means connecting
  them together so that they form a single larger program. If
  either part is covered by the GPL, the whole combination must
  also be released under the GPL--if you can't, or won't, do
  that, you may not combine them.". Is it true ?
	Yes. But the GPL FAQ speak of a generic GPL program without
	any exception.

	The license exception explicitly allows you to combine the
	GPL program with MAME. These exceptions exist specifically
	to solve this problem. Essentially, if your GPL program need
	to use another not GPL module, you (as copyright holder) can
	grant at the user the rights to link it with the not GPL
	module.

	The GPL was made to protect the interests of the copyright
	holder. But if the copyright holder wants, he can relax the
	license adding specific exceptions that extend the rights of
	the users. So, the users can do things not generally
	permitted by the simple GPL license.

  Which is the license of the AdvanceMAME binary ?
	The AdvanceMAME DOS binary contains source from AdvanceMAME,
	MAME, Allegro, SEAL, svgalib and zlib. You must follow ALL
	these licenses when you use the binary. So, you can do with
	it only things allowed by ALL these licenses.

  Can I sell the AdvanceMAME binary present on the web site ?
	No. The AdvanceMAME binary contains compiled MAME source and
	the "MAME License" proibits selling.

  Who is the copyright holder of AdvanceMAME ?
	Andrea Mazzoleni is the copyright holder of AdvanceMAME.

	Some source and documentation files are also under the
	copyright of Filipe Estima, Ian Patterson, Randy Schnedler,
	S. Sakamaki.

GPL FAQ
	This is an extract of the Official GPL FAQ present at :

		http://www.gnu.org/licenses/gpl-faq.html#WritingFSWithNFLibs

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

Copyright
	This file is Copyright (C) 2002 Andrea Mazzoleni.

