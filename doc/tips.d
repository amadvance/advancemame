NAME
	tips - The Speed Tips

	This is a collection of tips to improve the emulation speed.

Speed Tips That Don't Reduce The Emulation Quality
	The following are simple tips to improve the speed of the
	emulation without any quality degradation.

    Disk Cache
	Ensure to have a disk cache loaded. If you are using DOS load
	the SMARTDRV utility. If you are short in memory use a very
	small cache like 1MB.

    System memory
	If you are using DOS ensure that you are using the whole
	system memory. Incorrect configuration may result in upper
	memory limit of 32/64/128 MB.

	With a correct configuration the physical memory available
	must be approximatively large as your system memory less the
	memory used by the SMARTDRV cache and the 10 MB of the
	executable size (not compressed).
	If this doesn't happen check the DOS faq for a correct DOS
	configuration.

    Video memory bandwidth
	Enable the "PCI write combining" feature for your video board
	"Linear Frame Buffer".
	In DOS and Windows try with the FASTVID utility from:

		http://www.fastgraphics.com

	In Linux you need to activate the MTRR support of your kernel
	and configure it. Check the documentation in:

		file://usr/src/linux/Documentation/mtrr.txt

	You can test the effects of the changes using the `advv'
	utility pressing F9 on a standard video mode like
	"640x480x60Hz 8 bit".
	The number on the upper left corner if the memory bandwidth in
	MB/s. Good values are over 200 MB/s.

Speed Tips That Reduce The Emulation Quality
	The following are simple tips to improve the speed of the
	emulation with a quality degradation.

    Sound rate
	Select a lower sample rate than the default 44100 with the
	option :

		:sound_samplerate 22050

	A more aggressive value is 11025.

    Video depth
	Force the use of the 8 bit depth with the option :

		:display_depth 8

Copyright
	This file is Copyright (C) 2003 Andrea Mazzoleni.

