/******************************************************************************

    mamedriv.c

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

    The list of all available drivers. Drivers have to be included here to be
    recognized by the executable.

    To save some typing, we use a hack here. This file is recursively #included
    twice, with different definitions of the DRIVER() macro. The first one
    declares external references to the drivers; the second one builds an array
    storing all the drivers.

******************************************************************************/

#include "driver.h"

#ifndef DRIVER_RECURSIVE

#define DRIVER_RECURSIVE

/* step 1: declare all external references */
#define DRIVER(NAME) extern game_driver driver_##NAME;
#include "mamedriv.c"

/* step 2: define the drivers[] array */
#undef DRIVER
#define DRIVER(NAME) &driver_##NAME,
const game_driver * const drivers[] =
{
#include "mamedriv.c"
	0	/* end of array */
};

#else	/* DRIVER_RECURSIVE */

	/* "Pacman hardware" games */
	DRIVER( puckman )	/* (c) 1980 Namco */
	DRIVER( puckmana )	/* (c) 1980 Namco */
	DRIVER( puckmanf )	/* hack */
	DRIVER( puckmanh )	/* hack */
	DRIVER( pacman )	/* (c) 1980 Midway */
	DRIVER( pacmanf )	/* hack */
	DRIVER( puckmod )	/* (c) 1981 Namco */
	DRIVER( pacmod )	/* (c) 1981 Midway */
	DRIVER( newpuc2 )	/* hack */
	DRIVER( newpuc2b )	/* hack */
	DRIVER( newpuckx )	/* hack */
	DRIVER( pacheart )	/* hack */
	DRIVER( hangly )	/* hack */
	DRIVER( hangly2 )	/* hack */
	DRIVER( hangly3 )	/* hack */
	DRIVER( piranhah )	/* hack */
	DRIVER( abscam )	/* hack */
	DRIVER( crush )		/* (c) 1981 Kural Samno Electric Ltd */
	DRIVER( crush2 )	/* (c) 1981 Kural Esco Electric Ltd - bootleg? */
	DRIVER( crush3 )	/* Kural Electric Ltd - bootleg? */
	DRIVER( crush4 )	/* Kural TWT */
	DRIVER( maketrax )	/* (c) 1981 Williams, high score table says KRL (fur Kural) */
	DRIVER( maketrxb )	/* (c) 1981 [Kural] (Williams license) */
	DRIVER( korosuke )	/* (c) 1981 Kural Electric */
	DRIVER( mbrush )	/* 1981 bootleg */
	DRIVER( paintrlr )	/* 1981 bootleg */
	DRIVER( pacplus )
	DRIVER( joyman )	/* hack */
	DRIVER( ctrpllrp )	/* hack */
	DRIVER( eyes )		/* (c) 1982 Digitrex Techstar + "Rockola presents" */
	DRIVER( eyes2 )		/* (c) 1982 Techstar + "Rockola presents" */
	DRIVER( eyeszac )	/* (c) 1982 Zaccaria / bootleg */
	DRIVER( mrtnt )		/* (c) 1983 Telko */
	DRIVER( gorkans )	/* (c) 1984 Techstar */
	DRIVER( eggor )		/* (c) 1983 Telko */
	DRIVER( jumpshot )	/* (c) 1985 Bally Midway */
	DRIVER( jumpshtp )	/* (c) 1985 Bally Midway */
	DRIVER( shootbul )	/* (c) 1985 Bally Midway */
	DRIVER( piranha )	/* GL */
	DRIVER( piranhao )	/* GL */
	DRIVER( nmouse )	/* (c) 1981 Amenip (Palcom Queen River) */
	DRIVER( nmouseb )	/* (c) 1981 Amenip Nova Games Ltd. */
	DRIVER( mspacman )	/* (c) 1981 Midway */ /* made by Gencomp */
	DRIVER( mspacmnf )	/* hack */
	DRIVER( mspacmat )	/* hack */
	DRIVER( woodpek )
	DRIVER( woodpeka )
	DRIVER( mspacmab )	/* bootleg */
	DRIVER( mspacmbe )	/* bootleg */
	DRIVER( pacgal )	/* hack */
	DRIVER( mspacpls )	/* hack */
	DRIVER( ponpoko )	/* (c) 1982 Sigma Ent. Inc. */
	DRIVER( ponpokov )	/* (c) 1982 Sigma Ent. Inc. + Venture Line license */
	DRIVER( lizwiz )	/* (c) 1985 Techstar + "Sunn presents" */
	DRIVER( alibaba )	/* (c) 1982 Sega */
	DRIVER( dremshpr )	/* (c) 1982 Sanritsu */
	DRIVER( vanvan )	/* (c) 1983 Sanritsu */
	DRIVER( vanvank )	/* (c) 1983 Karateco (bootleg?) */
	DRIVER( vanvanb )	/* (c) 1983 Karateco (bootleg?) */
	DRIVER( bwcasino )	/* (c) 1983 Epos Coropration */
	DRIVER( acitya )	/* (c) 1983 Epos Corporation */
	DRIVER( theglobp )	/* (c) 1983 Epos Corporation */
	DRIVER( sprglobp )	/* (c) 1983 Epos Corporation */
	DRIVER( sprglbpg )	/* German bootleg */
	DRIVER( beastf )	/* (c) 1984 Epos Corporation */
	DRIVER( rocktrv2 )	/* (c) 1986 Triumph Software Inc */
	DRIVER( bigbucks )	/* (c) 1986 Dynasoft Inc. */
	DRIVER( mschamp )	/* hack */

	/* S2650 Pacman Kits */
	DRIVER( drivfrcp )	/* (c) 1984 Shinkai Inc. (Magic Eletronics Inc. licence) */
	DRIVER( 8bpm )		/* (c) 1985 Seatongrove Ltd (Magic Eletronics USA licence) */
	DRIVER( porky )		/* (c) 1985 Shinkai Inc. (Magic Eletronics Inc. licence) */

	DRIVER( pengo )		/* 834-0386 (c) 1982 Sega */
	DRIVER( pengo2 )	/* 834-0386 (c) 1982 Sega */
	DRIVER( pengo2u )	/* 834-0386 (c) 1982 Sega */
	DRIVER( pengo3u )	/* 834-0386 (c) 1982 Sega */
	DRIVER( pengo4 )	/* 834-0386 (c) 1982 Sega */
	DRIVER( pengob )	/* bootleg */
	DRIVER( penta )		/* bootleg */
	DRIVER( jrpacmbl )	/* bootleg on Pengo HW */

	DRIVER( jrpacman )	/* (c) 1983 Midway */

	/* Epos games */
	DRIVER( megadon )	/* (c) 1982 */
	DRIVER( catapult )	/* (c) 1982 */
	DRIVER( suprglob )	/* (c) 1983 */
	DRIVER( theglob )	/* (c) 1983 */
	DRIVER( theglob2 )	/* (c) 1983 */
	DRIVER( theglob3 )	/* (c) 1983 */
	DRIVER( igmo )		/* (c) 1984 */
	DRIVER( dealer )	/* (c) 198? */
	DRIVER( revenger )	/* (c) 1984 */

	/* "Galaxian hardware" games */
	DRIVER( galaxian )	/* (c) Namco */
	DRIVER( galaxiaj )	/* (c) Namco */
	DRIVER( galmidw )	/* (c) Midway */
	DRIVER( galmidwo )	/* (c) Midway */
	DRIVER( superg )	/* hack */
	DRIVER( galapx )	/* hack */
	DRIVER( moonaln )	/* [Nichibutsu] (Karateco license) or hack */
	DRIVER( galap1 )	/* hack */
	DRIVER( galap4 )	/* hack */
	DRIVER( galturbo )	/* hack */
	DRIVER( swarm )		/* hack */
	DRIVER( zerotime )	/* hack */
	DRIVER( starfght )	/* hack */
	DRIVER( tst_galx )
	DRIVER( gmgalax )	/* bootleg */
	DRIVER( pisces )	/* Subelectro */
	DRIVER( piscesb )	/* bootleg */
	DRIVER( omni )		/* bootleg */
	DRIVER( uniwars )	/* (c) Irem */
	DRIVER( gteikoku )	/* (c) Irem */
	DRIVER( gteikokb )	/* bootleg */
	DRIVER( gteikob2 )	/* bootleg */
	DRIVER( spacbatt )	/* bootleg */
	DRIVER( skyraidr )	/* bootleg */
	DRIVER( batman2 )	/* bootleg */
	DRIVER( warofbug )	/* (c) 1981 Armenia */
	DRIVER( redufo )	/* bootleg - original should be (c) Artic */
	DRIVER( exodus )	/* Subelectro - bootleg? */
	DRIVER( streakng )	/* [1980] Shoei */
	DRIVER( pacmanbl )	/* bootleg */
	DRIVER( devilfsg )	/* (c) 1984 Vision / Artic (bootleg?) */
	DRIVER( zigzag )	/* (c) 1982 LAX */
	DRIVER( zigzag2 )	/* (c) 1982 LAX */
	DRIVER( jumpbug )	/* (c) 1981 Rock-ola */
	DRIVER( jumpbugb )	/* (c) 1981 Sega */
	DRIVER( levers )	/* (c) 1983 Rock-ola */
	DRIVER( azurian )	/* (c) 1982 Rait Electronics Ltd */
	DRIVER( orbitron )	/* Signatron USA */
	DRIVER( mooncrgx )	/* bootleg */
	DRIVER( mooncrst )	/* (c) 1980 Nichibutsu */
	DRIVER( mooncrsu )	/* (c) 1980 Nichibutsu USA */
	DRIVER( mooncrsa )	/* (c) 1980 Nichibutsu */
	DRIVER( mooncrsg )	/* (c) 1980 Gremlin */
	DRIVER( smooncrs )	/* Gremlin */
	DRIVER( mooncrsb )	/* bootleg */
	DRIVER( mooncrs2 )	/* bootleg */
	DRIVER( mooncrs3 )	/* bootleg */
	DRIVER( fantazia )	/* bootleg */
	DRIVER( moonqsr )	/* (c) 1980 Nichibutsu */
	DRIVER( mshuttle )	/* (c) 1981 Nichibutsu */
	DRIVER( mshuttlj )	/* (c) 1981 Nichibutsu */
	DRIVER( mshutlj2 )	/* (c) 1981 Nichibutsu */
	DRIVER( moonal2 )	/* Nichibutsu */
	DRIVER( moonal2b )	/* Nichibutsu */
	DRIVER( supergx )	/* ? */
	DRIVER( eagle )		/* (c) Centuri */
	DRIVER( eagle2 )	/* (c) Centuri */
	DRIVER( eagle3 )	/* (c) Centuri */
	DRIVER( spctbird )	/* (c) Fortrek */
	DRIVER( skybase )	/* (c) 1982 Omori Electric Co., Ltd. */
	DRIVER( checkman )	/* (c) 1982 Zilec-Zenitone */
	DRIVER( checkmaj )	/* (c) 1982 Jaleco (Zenitone/Zilec in ROM CM4, and the programmer names) */
	DRIVER( dingo )		/* (c) 1983 Ashby Computers and Graphics LTD. + Jaleco license */
	DRIVER( dingoe )	/* (c) 1983 Ashby Computers and Graphics LTD. + Jaleco license */
	DRIVER( blkhole )	/* TDS (Tokyo Denshi Sekkei) */
	DRIVER( kingball )	/* (c) 1980 Namco */
	DRIVER( kingbalj )	/* (c) 1980 Namco */
	DRIVER( scorpnmc )	/* bootleg */
	DRIVER( frogg )		/* bootleg */
	DRIVER( 4in1 )		/* (c) 1981 Armenia / Food and Fun */
	DRIVER( bagmanmc )	/* bootleg */
	DRIVER( dkongjrm )	/* bootleg */
	DRIVER( ozon1 )		/* (c) 1983 Proma */
	DRIVER( ladybugg )	/* bootleg */
	DRIVER( vpool )		/* bootleg */
	DRIVER( drivfrcg )	/* Shinkai */
	DRIVER( drivfrcb )	/* bootleg */
	DRIVER( bongo )		/* Jetsoft */
	DRIVER( hunchbkg )	/* Century */
	DRIVER( harem )		/* I.G.R. */
	DRIVER( tazzmang )
	DRIVER( racknrol )	/* Status (Senko Kit) */
	DRIVER( hexpool )	/* Status (Senko Kit) */
	DRIVER( trvchlng )	/* Senko */
	DRIVER( luctoday )	/* 1980 Sigma */
	DRIVER( chewing )	/* ?? unknown */

	/* Has some similarities with Moon Cresta but Board is very different */
	DRIVER( rockclim )	/* (c)1981 Taito */

	/* "Scramble hardware" (and variations) games */
	DRIVER( scramble )	/* GX387 (c) 1981 Konami */
	DRIVER( scrambls )	/* GX387 (c) 1981 Stern */
	DRIVER( scramblb )	/* bootleg */
	DRIVER( explorer )	/* bootleg */
	DRIVER( strfbomb )	/* bootleg */
	DRIVER( atlantis )	/* (c) 1981 Comsoft */
	DRIVER( atlants2 )	/* (c) 1981 Comsoft */
	DRIVER( theend )	/* (c) 1980 Konami */
	DRIVER( theends )	/* (c) 1980 Stern */
	DRIVER( omega )		/* bootleg */
	DRIVER( ckongs )	/* bootleg */
	DRIVER( froggers )	/* bootleg */
	DRIVER( frogf )		/* bootleg */
	DRIVER( amidars )	/* GX337 (c) 1982 Konami */
	DRIVER( triplep )	/* (c) 1982 KKI */	/* made by Sanritsu? */
	DRIVER( knockout )	/* (c) 1982 KKK */
	DRIVER( mariner )	/* (c) 1981 Amenip */
	DRIVER( 800fath )	/* (c) 1981 Amenip + U.S. Billiards license */
	DRIVER( mars )		/* (c) 1981 Artic */
	DRIVER( devilfsh )	/* (c) 1982 Artic */
	DRIVER( newsin7 )	/* (c) 1983 ATW USA, Inc. */
	DRIVER( mrkougar )	/* (c) 1984 ATW */
	DRIVER( mrkougr2 )
	DRIVER( mrkougb )	/* bootleg */
	DRIVER( mrkougb2 )	/* bootleg */
	DRIVER( hotshock )	/* (c) 1982 E.G. Felaco */
	DRIVER( conquer )	/* (c) 1982 ???? */
	DRIVER( hunchbks )	/* (c) 1983 Century */
	DRIVER( hncholms )
	DRIVER( cavelon )	/* (c) 1983 Jetsoft */
	DRIVER( sfx )		/* (c) 1983 Nichibutsu */
	DRIVER( skelagon )	/* (c) 1983 Nichibutsu USA */
	DRIVER( mimonscr )
	DRIVER( mimonkey )
	DRIVER( mimonsco )
	DRIVER( scobra )	/* GX316 (c) 1981 Konami */
	DRIVER( scobras )	/* GX316 (c) 1981 Stern */
	DRIVER( scobrase )	/* GX316 (c) 1981 Stern */
	DRIVER( scobrab )	/* GX316 (c) 1981 Karateco (bootleg?) */
	DRIVER( stratgyx )	/* GX306 (c) 1981 Konami */
	DRIVER( stratgys )	/* GX306 (c) 1981 Stern */
	DRIVER( armorcar )	/* (c) 1981 Stern */
	DRIVER( armorca2 )	/* (c) 1981 Stern */
	DRIVER( moonwar )	/* (c) 1981 Stern */
	DRIVER( moonwara )	/* (c) 1981 Stern */
	DRIVER( spdcoin )	/* (c) 1984 Stern */
	DRIVER( darkplnt )	/* (c) 1982 Stern */
	DRIVER( tazmania )	/* (c) 1982 Stern */
	DRIVER( tazmani2 )	/* (c) 1982 Stern */
	DRIVER( calipso )	/* (c) 1982 Tago */
	DRIVER( anteater )	/* (c) 1982 Tago */
	DRIVER( anteatg )	/* (c) ? */
	DRIVER( anteatgb )	/* (c) ? */
	DRIVER( rescue )	/* (c) 1982 Stern */
	DRIVER( aponow )	/* bootleg */
	DRIVER( minefld )	/* (c) 1983 Stern */
	DRIVER( losttomb )	/* (c) 1982 Stern */
	DRIVER( losttmbh )	/* (c) 1982 Stern */
	DRIVER( superbon )	/* bootleg */
	DRIVER( hustler )	/* GX343 (c) 1981 Konami */
	DRIVER( billiard )	/* bootleg */
	DRIVER( hustlerb )	/* bootleg */
	DRIVER( frogger )	/* GX392 (c) 1981 Konami */
	DRIVER( frogseg1 )	/* (c) 1981 Sega */
	DRIVER( frogseg2 )	/* 834-0068 (c) 1981 Sega */
	DRIVER( froggrmc )	/* 800-3110 (c) 1981 Sega */
	DRIVER( amidar )	/* GX337 (c) 1981 Konami */
	DRIVER( amidaru )	/* GX337 (c) 1982 Konami + Stern license */
	DRIVER( amidaro )	/* GX337 (c) 1982 Konami + Olympia license */
	DRIVER( amidarb )	/* bootleg */
	DRIVER( amigo )		/* bootleg */
	DRIVER( turtles )	/* (c) 1981 Stern */
	DRIVER( turpin )	/* (c) 1981 Sega */
	DRIVER( 600 )		/* GX353 (c) 1981 Konami */
	DRIVER( flyboy )	/* (c) 1982 Kaneko */
	DRIVER( flyboyb )	/* bootleg */
	DRIVER( fastfred )	/* 136009   (c) 1982 Atari */
	DRIVER( jumpcoas )	/* (c) 1983 Kaneko */
	DRIVER( jumpcoat )	/* (c) 1983 Taito */
	DRIVER( boggy84 )	/* bootleg, original is (c)1983 Taito/Kaneko */
	DRIVER( redrobin )	/* (c) 1986 Elettronolo */
	DRIVER( imago )
	DRIVER( scorpion )	/* (c) 1982 Zaccaria */
	DRIVER( scrpiona )	/* (c) 1982 Zaccaria */
	DRIVER( ad2083 )	/* (c) 1983 Midcoin */

	/* "Crazy Climber hardware" games */
	DRIVER( cclimber )	/* (c) 1980 Nichibutsu */
	DRIVER( cclimbrj )	/* (c) 1980 Nichibutsu */
	DRIVER( ccboot )	/* bootleg */
	DRIVER( ccboot2 )	/* bootleg */
	DRIVER( ckong )		/* (c) 1981 Falcon */
	DRIVER( ckonga )	/* (c) 1981 Falcon */
	DRIVER( ckongjeu )	/* bootleg */
	DRIVER( ckongo )	/* bootleg */
	DRIVER( ckongalc )	/* bootleg */
	DRIVER( bigkong )	/* bootleg */
	DRIVER( monkeyd )	/* bootleg */
	DRIVER( rpatrolb )	/* bootleg */
	DRIVER( rpatrolo )	/* Orca, from a suspicious set */
	DRIVER( silvland )	/* Falcon */
	DRIVER( yamato )	/* (c) 1983 Sega */
	DRIVER( yamato2 )	/* (c) 1983 Sega */
	DRIVER( toprollr )	/* (c) 1983 Jaleco */
	DRIVER( swimmer )	/* (c) 1982 Tehkan */
	DRIVER( swimmera )	/* (c) 1982 Tehkan */
	DRIVER( swimmerb )	/* (c) 1982 Tehkan */
	DRIVER( guzzler )	/* (c) 1983 Tehkan */
	DRIVER( cannonb )	/* (c) 1985 Soft */
	DRIVER( cannonb2 )	/* (c) 1985 TV Game Gruenberg */
	DRIVER( ckongb )	/* bootleg */

	/* Nichibutsu games */
	DRIVER( gomoku )	/* (c) 1981 */
	DRIVER( wiping )	/* (c) 1982 */
	DRIVER( rugrats )	/* (c) 1983 */
	DRIVER( dacholer )	/* (c) 1983 */
	DRIVER( kickboy )	/* (c) 1983 */
	DRIVER( friskyt )	/* (c) 1981 */
	DRIVER( friskyta )	/* (c) 1981 */
	DRIVER( radrad )	/* (c) 1982 Nichibutsu USA */
	DRIVER( seicross )	/* (c) 1984 + Alice */
	DRIVER( sectrzon )	/* (c) 1984 + Alice */
	DRIVER( firebatl )	/* (c) 1984 Taito */
	DRIVER( clshroad )	/* (c) 1986 Woodplace Inc. */
	DRIVER( clshrdst )	/* (c) 1986 Status Games */
	DRIVER( tubep )		/* (c) 1984 + Fujitek */
	DRIVER( rjammer )	/* (c) 1984 + Alice */
	DRIVER( magmax )	/* (c) 1985 */
	DRIVER( cop01 )		/* (c) 1985 */
	DRIVER( cop01a )	/* (c) 1985 */
	DRIVER( mightguy )	/* (c) 1986 */
	DRIVER( terracre )	/* (c) 1985 */
	DRIVER( terracrb )	/* (c) 1985 */
	DRIVER( terracra )	/* (c) 1985 */
	DRIVER( amazon )	/* (c) 1986 */
	DRIVER( amatelas )	/* (c) 1986 */
	DRIVER( horekid )	/* (c) 1987 */
	DRIVER( horekidb )	/* bootleg */
	DRIVER( galivan )	/* (c) 1985 */
	DRIVER( galivan2 )	/* (c) 1985 */
	DRIVER( dangar )	/* (c) 1986 */
	DRIVER( dangar2 )	/* (c) 1986 */
	DRIVER( dangarb )	/* bootleg */
	DRIVER( ninjemak )	/* (c) 1986 (US?) */
	DRIVER( youma )		/* (c) 1986 (Japan) */
	DRIVER( legion )	/* (c) 1986 */
	DRIVER( legiono )	/* (c) 1986 */
	DRIVER( terraf )	/* (c) 1987 */
	DRIVER( terrafu )	/* (c) 1987 Nichibutsu USA */
	DRIVER( kodure )	/* (c) 1987 (Japan) */
	DRIVER( armedf )	/* (c) 1988 */
	DRIVER( cclimbr2 )	/* (c) 1988 (Japan) */
	DRIVER( cclmbr2a )	/* (c) 1988 (Japan) */
	DRIVER( bigfghtr )	/* (c) 1989 */

	/* Nichibutsu Mahjong games */
	DRIVER( hyhoo )		/* (c) 1987 */
	DRIVER( hyhoo2 )	/* (c) 1987 */

	DRIVER( pastelg )	/* (c) 1985 */

	DRIVER( crystalg )	/* (c) 1986 */
	DRIVER( crystal2 )	/* (c) 1986 */
	DRIVER( nightlov )	/* (c) 1986 */
	DRIVER( citylove )	/* (c) 1986 */
	DRIVER( mcitylov )	/* (c) 1986 */
	DRIVER( apparel )	/* (c) 1986 Central Denshi */
	DRIVER( secolove )	/* (c) 1986 */
	DRIVER( housemnq )	/* (c) 1987 */
	DRIVER( housemn2 )	/* (c) 1987 */
	DRIVER( livegal )	/* (c) 1987 */
	DRIVER( seiha )		/* (c) 1987 */
	DRIVER( seiham )	/* (c) 1987 */
	DRIVER( bijokkoy )	/* (c) 1987 */
	DRIVER( iemoto )	/* (c) 1987 */
	DRIVER( iemotom )	/* (c) 1987 */
	DRIVER( ryuuha )	/* (c) 1987 */
	DRIVER( ojousan )	/* (c) 1987 */
	DRIVER( ojousanm )	/* (c) 1987 */
	DRIVER( bijokkog )	/* (c) 1988 */
	DRIVER( orangec )	/* (c) 1988 Daiichi Denshi */
	DRIVER( orangeci )	/* (c) 1988 Daiichi Denshi */
	DRIVER( vipclub )	/* (c) 1988 Daiichi Denshi */
	DRIVER( korinai )	/* (c) 1988 */
	DRIVER( korinaim )	/* (c) 1988 */
	DRIVER( kaguya )	/* (c) 1988 MIKI SYOUJI */
	DRIVER( kaguya2 )	/* (c) 1989 MIKI SYOUJI */
	DRIVER( kaguya2f )	/* (c) 1989 MIKI SYOUJI */
	DRIVER( otonano )	/* (c) 1988 Apple */
	DRIVER( kanatuen )	/* (c) 1988 Panac */
	DRIVER( kyuhito )	/* (c) 1988 ROLLER TRON */
	DRIVER( mjsikaku )	/* (c) 1988 */
	DRIVER( mjsikakb )	/* (c) 1988 */
	DRIVER( mjsikakc )	/* (c) 1988 */
	DRIVER( mjsikakd )	/* (c) 1988 */
	DRIVER( mmsikaku )	/* (c) 1988 */
	DRIVER( mjcamera )	/* (c) 1988 MIKI SYOUJI */
	DRIVER( mmcamera )	/* (c) 1988 MIKI SYOUJI */

	DRIVER( msjiken )	/* (c) 1988 */
	DRIVER( hanamomo )	/* (c) 1988 */
	DRIVER( hanamomb )	/* (c) 1988 */
	DRIVER( telmahjn )	/* (c) 1988 */
	DRIVER( taiwanmb )	/* (c) 1988 */
	DRIVER( gionbana )	/* (c) 1989 */
	DRIVER( idhimitu )	/* (c) 1989 Digital Soft */
	DRIVER( mgion )		/* (c) 1989 */
	DRIVER( mjcamerb )	/* (c) 1989 MIKI SYOUJI */
	DRIVER( omotesnd )	/* (c) 1989 */
	DRIVER( abunai )	/* (c) 1989 */
	DRIVER( mjfocus )	/* (c) 1989 */
	DRIVER( mjfocusm )	/* (c) 1989 */
	DRIVER( peepshow )	/* (c) 1989 AC */
	DRIVER( scandal )	/* (c) 1989 */
	DRIVER( scandalm )	/* (c) 1989 */
	DRIVER( mgmen89 )	/* (c) 1989 */
	DRIVER( mjnanpas )	/* (c) 1989 BROOKS */
	DRIVER( mjnanpaa )	/* (c) 1989 BROOKS */
	DRIVER( mjnanpau )	/* (c) 1989 BROOKS */
	DRIVER( pairsnb )	/* (c) 1989 */
	DRIVER( pairsten )	/* (c) 1989 System Ten */
	DRIVER( bananadr )	/* (c) 1989 DIGITAL SOFT */
	DRIVER( mladyhtr )	/* (c) 1990 */
	DRIVER( chinmoku )	/* (c) 1990 */
	DRIVER( maiko )		/* (c) 1990 */
	DRIVER( mmaiko )	/* (c) 1990 */
	DRIVER( club90s )	/* (c) 1990 */
	DRIVER( club90sa )	/* (c) 1990 */
	DRIVER( lovehous )	/* (c) 1990 */
	DRIVER( hanaoji )	/* (c) 1991 */

	DRIVER( pstadium )	/* (c) 1990 */
	DRIVER( triplew1 )	/* (c) 1989 */
	DRIVER( triplew2 )	/* (c) 1990 */
	DRIVER( ntopstar )	/* (c) 1990 */
	DRIVER( mjlstory )	/* (c) 1991 */
	DRIVER( vanilla )	/* (c) 1991 */
	DRIVER( finalbny )	/* (c) 1991 */
	DRIVER( qmhayaku )	/* (c) 1991 */
	DRIVER( mjgottub )	/* (c) 1991 */
	DRIVER( galkoku )	/* (c) 1989 Nichibutsu/T.R.TEC */
	DRIVER( hyouban )	/* (c) 1989 Nichibutsu/T.R.TEC */
	DRIVER( galkaika )	/* (c) 1989 Nichibutsu/T.R.TEC */
	DRIVER( tokyogal )	/* (c) 1989 */
	DRIVER( tokimbsj )	/* (c) 1989 */
	DRIVER( mcontest )	/* (c) 1989 */
	DRIVER( uchuuai )	/* (c) 1989 */
	DRIVER( av2mj1bb )	/* (c) 1991 MIKI SYOUJI/AV JAPAN */
	DRIVER( av2mj2rg )	/* (c) 1991 MIKI SYOUJI/AV JAPAN */

	DRIVER( mjuraden )	/* (c) 1992 Nichibutsu/Yubis */
	DRIVER( koinomp )	/* (c) 1992 */
	DRIVER( patimono )	/* (c) 1992 */
	DRIVER( janbari )	/* (c) 1992 Nichibutsu/Yubis/AV JAPAN */
	DRIVER( mjanbari )	/* (c) 1992 Nichibutsu/Yubis/AV JAPAN */
	DRIVER( ultramhm )	/* (c) 1993 Apple */
	DRIVER( gal10ren )	/* (c) 1993 FUJIC */
	DRIVER( renaiclb )	/* (c) 1993 FUJIC */
	DRIVER( mjlaman )	/* (c) 1993 Nichibutsu/AV JAPAN */
	DRIVER( mkeibaou )	/* (c) 1993 */
	DRIVER( pachiten )	/* (c) 1993 Nichibutsu/MIKI SYOUJI/AV JAPAN */
	DRIVER( sailorws )	/* (c) 1993 */
	DRIVER( sailorwa )	/* (c) 1993 */
	DRIVER( sailorwr )	/* (c) 1993 */
	DRIVER( wcatcher )	/* (c) 1993 */
	DRIVER( jituroku )	/* (c) 1993 Windom */
	DRIVER( yosimoto )	/* (c) 1994 Nichibutsu/Yoshimoto Kougyou */
	DRIVER( psailor1 )	/* (c) 1994 SPHINX */
	DRIVER( psailor2 )	/* (c) 1994 SPHINX */
	DRIVER( otatidai )	/* (c) 1995 SPHINX */
	DRIVER( ngpgal )	/* (c) 1991 */
	DRIVER( mjgottsu )	/* (c) 1991 */
	DRIVER( bakuhatu )	/* (c) 1991 */
	DRIVER( cmehyou )	/* (c) 1992 Nichibutsu/Kawakusu */
	DRIVER( mmehyou )	/* (c) 1992 Nichibutsu/Kawakusu */
	DRIVER( mjkoiura )	/* (c) 1992 */
	DRIVER( mkoiuraa )	/* (c) 1992 */
	DRIVER( imekura )	/* (c) 1994 SPHINX/AV JAPAN */
	DRIVER( mscoutm )	/* (c) 1994 SPHINX/AV JAPAN */
	DRIVER( mjegolf )	/* (c) 1994 FUJIC/AV JAPAN */

	DRIVER( mhhonban )	/* (c) 199? */
	DRIVER( 4psimasy )	/* (c) 1994 */
	DRIVER( musobana )	/* (c) 1995 */
	DRIVER( niyanpai )	/* (c) 1996 */

	/* "Phoenix hardware" (and variations) games */
	DRIVER( safarir )	/* Shin Nihon Kikaku (SNK) */
	DRIVER( phoenix )	/* (c) 1980 Amstar */
	DRIVER( phoenixa )	/* (c) 1980 Amstar + Centuri license */
	DRIVER( phoenixt )	/* (c) 1980 Taito */
	DRIVER( phoenix3 )	/* bootleg */
	DRIVER( phoenixc )	/* bootleg */
	DRIVER( condor )	/* bootleg */
	DRIVER( falcon )	/* bootleg */
	DRIVER( vautour )	/* bootleg (Jeutel) */
	DRIVER( vautour2 )	/* bootleg */
	DRIVER( griffon )	/* bootleg (Videotron) */
	DRIVER( falcona )	/* bootleg */
	DRIVER( pleiads )	/* (c) 1981 Tehkan */
	DRIVER( pleiadbl )	/* bootleg */
	DRIVER( pleiadce )	/* (c) 1981 Centuri + Tehkan */
	DRIVER( capitol )	/* bootleg? */
	DRIVER( survival )	/* (c) 1982 Rock-ola */
	DRIVER( naughtyb )	/* (c) 1982 Jaleco */
	DRIVER( naughtya )	/* bootleg */
	DRIVER( naughtyc )	/* (c) 1982 Jaleco + Cinematronics */
	DRIVER( popflame )	/* (c) 1982 Jaleco */
	DRIVER( popflama )	/* (c) 1982 Jaleco */
	DRIVER( popflamb )	/* (c) 1982 Jaleco */
	DRIVER( trvmstr )	/* (c) 1985 Enerdyne Technologies Inc. */
	DRIVER( trvmstra )	/* (c) 1985 Enerdyne Technologies Inc. */
	DRIVER( trvgns )	/* (c) 198? Enerdyne Technologies Inc. */



	/* Namco games (plus some intruders on similar hardware) */
	DRIVER( geebee )	/* [1978] Namco */
	DRIVER( geebeeg )	/* [1978] Gremlin */
	DRIVER( bombbee )	/* [1979] Namco */
	DRIVER( cutieq )	/* (c) 1979 Namco */
	DRIVER( navarone )	/* (c) 1980 Namco */
	DRIVER( kaitei )	/* [1980] Namco */
	DRIVER( kaiteik )	/* [1980] K.K. Tokki (prototype?) */
	DRIVER( sos )		/* [1980] Namco */
	DRIVER( tankbatt )	/* (c) 1980 Namco */
	DRIVER( warpwarp )	/* (c) 1981 Namco */
	DRIVER( warpwarr )	/* (c) 1981 Rock-ola - the high score table says "NAMCO" */
	DRIVER( warpwar2 )	/* (c) 1981 Rock-ola - the high score table says "NAMCO" */
	DRIVER( rallyx )	/* (c) 1980 Namco */
	DRIVER( rallyxm )	/* (c) 1980 Midway */
	DRIVER( nrallyx )	/* (c) 1981 Namco */
	DRIVER( jungler )	/* GX327 (c) 1981 Konami */
	DRIVER( junglers )	/* GX327 (c) 1981 Stern */
	DRIVER( tactcian )	/* GX335 (c) 1982 Sega */
	DRIVER( tactcan2 )	/* GX335 (c) 1981 Sega */
	DRIVER( locomotn )	/* GX359 (c) 1982 Konami + Centuri license */
	DRIVER( gutangtn )	/* GX359 (c) 1982 Konami + Sega license */
	DRIVER( cottong )	/* bootleg */
	DRIVER( commsega )	/* (c) 1983 Sega */
	/* the following ones all have a custom I/O chip */
	DRIVER( bosco )		/* (c) 1981 */
	DRIVER( boscoo )	/* (c) 1981 */
	DRIVER( boscoo2 )	/* (c) 1981 */
	DRIVER( boscomd )	/* (c) 1981 Midway */
	DRIVER( boscomdo )	/* (c) 1981 Midway */
	DRIVER( galaga )	/* (c) 1981 */
	DRIVER( galagao )	/* (c) 1981 */
	DRIVER( galagamw )	/* (c) 1981 Midway */
	DRIVER( galagamk )	/* (c) 1981 Midway */
	DRIVER( gallag )	/* bootleg */
	DRIVER( gatsbee )	/* (c) 1984 Uchida / hack */
	DRIVER( digdug )	/* (c) 1982 */
	DRIVER( digdugb )	/* (c) 1982 */
	DRIVER( digdugat )	/* 136007   (c) 1982 Atari */
	DRIVER( digduga1 )	/* 136007   (c) 1982 Atari */
	DRIVER( dzigzag )	/* bootleg */
	DRIVER( xevious )	/* (c) 1982 */
	DRIVER( xeviousa )	/* 136018   (c) 1982 + Atari license */
	DRIVER( xeviousb )	/* 136018   (c) 1982 + Atari license */
	DRIVER( xeviousc )	/* 136018   (c) 1982 + Atari license */
	DRIVER( xevios )	/* bootleg */
	DRIVER( battles )	/* bootleg */
	DRIVER( sxevious )	/* (c) 1984 */
	DRIVER( superpac )	/* (c) 1982 */
	DRIVER( superpcm )	/* (c) 1982 Midway */
	DRIVER( pacnpal )	/* (c) 1983 */
	DRIVER( pacnpal2 )	/* (c) 1983 */
	DRIVER( pacnchmp )	/* (c) 1983 */
	DRIVER( phozon )	/* (c) 1983 */
	DRIVER( mappy )		/* (c) 1983 */
	DRIVER( mappyj )	/* (c) 1983 */
	DRIVER( digdug2 )	/* (c) 1985 */
	DRIVER( digdug2o )	/* (c) 1985 */
	DRIVER( todruaga )	/* (c) 1984 */
	DRIVER( todruago )	/* (c) 1984 */
	DRIVER( motos )		/* (c) 1985 */
	DRIVER( grobda )	/* (c) 1984 */
	DRIVER( grobda2 )	/* (c) 1984 */
	DRIVER( grobda3 )	/* (c) 1984 */
	DRIVER( gaplus )	/* (c) 1984 */
	DRIVER( gaplusa )	/* (c) 1984 */
	DRIVER( gapluso )	/* (c) 1984 */
	DRIVER( galaga3 )	/* (c) 1984 */
	DRIVER( galaga3m )	/* (c) 1984 */
	DRIVER( galaga3a )	/* (c) 1984 */
	/* Libble Rabble board (first Japanese game using a 68000) */
	DRIVER( liblrabl )	/* (c) 1983 */
	DRIVER( toypop )	/* (c) 1986 */
	/* Z8000 games */
	DRIVER( polepos )	/* (c) 1982  */
	DRIVER( poleposa )	/* 136014   (c) 1982 + Atari license */
	DRIVER( polepos1 )	/* 136014   (c) 1982 Atari */
	DRIVER( topracer )	/* bootleg */
	DRIVER( topracra )	/* bootleg */
	DRIVER( topracrb )	/* bootleg */
	DRIVER( polepos2 )	/* (c) 1983 */
	DRIVER( poleps2a )	/* 136014   (c) 1983 + Atari license */
	DRIVER( poleps2b )	/* bootleg */
	/* no custom I/O in the following, HD63701 (or compatible) microcontroller instead */
	DRIVER( pacland )	/* (c) 1984 */
	DRIVER( pacland2 )	/* (c) 1984 */
	DRIVER( pacland3 )	/* (c) 1984 */
	DRIVER( paclandm )	/* (c) 1984 Midway */
	DRIVER( drgnbstr )	/* (c) 1984 */
	DRIVER( skykid )	/* (c) 1985 */
	DRIVER( skykido )	/* (c) 1985 */
	DRIVER( skykidd )	/* (c) 1985 */
	DRIVER( baraduke )	/* (c) 1985 */
	DRIVER( baraduka )	/* (c) 1985 */
	DRIVER( metrocrs )	/* (c) 1985 */
	DRIVER( metrocra )	/* (c) 1985 */

	/* Namco System 86 games */
	DRIVER( hopmappy )	/* (c) 1986 */
	DRIVER( skykiddx )	/* (c) 1986 */
	DRIVER( skykiddo )	/* (c) 1986 */
	DRIVER( roishtar )	/* (c) 1986 */
	DRIVER( genpeitd )	/* (c) 1986 */
	DRIVER( rthunder )	/* (c) 1986 new version */
	DRIVER( rthundro )	/* (c) 1986 old version */
	DRIVER( wndrmomo )	/* (c) 1987 */

	/* Thunder Ceptor HW */
	DRIVER( tceptor )	/* (c) 1986 */
	DRIVER( tceptor2 )	/* (c) 1986 */

	/* Namco System 1 games */
	DRIVER( shadowld )	/* (c) 1987 */
	DRIVER( youkaidk )	/* (c) 1987 (Japan new version) */
	DRIVER( yokaidko )	/* (c) 1987 (Japan old version) */
	DRIVER( dspirit )	/* (c) 1987 new version */
	DRIVER( dspirito )	/* (c) 1987 old version */
	DRIVER( blazer )	/* (c) 1987 (Japan) */
	DRIVER( quester )	/* (c) 1987 (Japan) */
	DRIVER( pacmania )	/* (c) 1987 */
	DRIVER( pacmanij )	/* (c) 1987 (Japan) */
	DRIVER( galaga88 )	/* (c) 1987 */
	DRIVER( galag88j )	/* (c) 1987 (Japan) */
	DRIVER( ws )		/* (c) 1988 (Japan) */
	DRIVER( berabohm )	/* (c) 1988 (Japan) */
	DRIVER( beraboho )	/* (c) 1988 (Japan) */
				/* 1988 Alice in Wonderland (English version of Marchen maze) */
	DRIVER( mmaze )		/* (c) 1988 (Japan) */
	DRIVER( bakutotu )	/* (c) 1988 (Japan) */
	DRIVER( wldcourt )	/* (c) 1988 (Japan) */
	DRIVER( splatter )	/* (c) 1988 (World) */
	DRIVER( splattej )	/* (c) 1988 (Japan) */
	DRIVER( faceoff )	/* (c) 1988 (Japan) */
	DRIVER( rompers )	/* (c) 1989 (Japan) */
	DRIVER( romperso )	/* (c) 1989 (Japan) */
	DRIVER( blastoff )	/* (c) 1989 (Japan) */
	DRIVER( ws89 )		/* (c) 1989 (Japan) */
	DRIVER( dangseed )	/* (c) 1989 (Japan) */
	DRIVER( ws90 )		/* (c) 1990 (Japan) */
	DRIVER( pistoldm )	/* (c) 1990 (Japan) */
	DRIVER( boxyboy )	/* (c) 1990 (US) */
	DRIVER( soukobdx )	/* (c) 1990 (Japan) */
	DRIVER( puzlclub )	/* (c) 1990 (Japan) */
	DRIVER( tankfrce )	/* (c) 1991 (US) */
	DRIVER( tankfrcj )	/* (c) 1991 (Japan) */

	/* Namco System 2 games */
	DRIVER( finallap )	/* 87.12 Final Lap */
	DRIVER( finalapd )	/* 87.12 Final Lap */
	DRIVER( finalapc )	/* 87.12 Final Lap */
	DRIVER( finlapjc )	/* 87.12 Final Lap */
	DRIVER( finlapjb )	/* 87.12 Final Lap */
	DRIVER( assault )	/* (c) 1988 */
	DRIVER( assaultj )	/* (c) 1988 (Japan) */
	DRIVER( assaultp )	/* (c) 1988 (Japan) */
	DRIVER( metlhawk )	/* (c) 1988 */
	DRIVER( metlhwkj )	/* (c) 1988 */
	DRIVER( ordyne )	/* (c) 1988 */
	DRIVER( ordynej )	/* (c) 1988 */
	DRIVER( mirninja )	/* (c) 1988 (Japan) */
	DRIVER( phelios )	/* (c) 1988 (Japan) */
	DRIVER( dirtfoxj )	/* (c) 1989 (Japan) */
	DRIVER( fourtrax )	/* 89.11 */
	DRIVER( valkyrie )	/* (c) 1989 (Japan) */
	DRIVER( finehour )	/* (c) 1989 (Japan) */
	DRIVER( burnforc )	/* (c) 1989 (Japan) */
	DRIVER( burnfrco )	/* (c) 1989 (Japan) */
	DRIVER( marvland )	/* (c) 1989 (US) */
	DRIVER( marvlanj )	/* (c) 1989 (Japan) */
	DRIVER( kyukaidk )	/* (c) 1990 (Japan) */
	DRIVER( kyukaido )	/* (c) 1990 (Japan) */
	DRIVER( dsaber )	/* (c) 1990 */
	DRIVER( dsaberj )	/* (c) 1990 (Japan) */
	DRIVER( finalap2 )	/* 90.8  Final Lap 2 */
	DRIVER( finalp2j )	/* 90.8  Final Lap 2 (Japan) */
	DRIVER( gollygho )	/* 91.7  Golly Ghost */
	DRIVER( rthun2 )	/* (c) 1990 */
	DRIVER( rthun2j )	/* (c) 1990 (Japan) */
	DRIVER( sgunner )	/* (c) 1990 */
	DRIVER( sgunnerj )	/* (c) 1990 */
	DRIVER( sgunner2 )	/* (c) 1991 (US) */
	DRIVER( sgunnr2j )	/* (c) 1991 (Japan) */
	DRIVER( cosmogng )	/* (c) 1991 (US) */
	DRIVER( cosmognj )	/* (c) 1991 (Japan) */
	DRIVER( bubbletr )	/* (c) 1992 */
	DRIVER( finalap3 )	/* 92.9  Final Lap 3 */
	DRIVER( finalp3j )	/* 92.9  Final Lap 3 */
	DRIVER( luckywld )	/* (c) 1992 */
	DRIVER( lckywldj )	/* (c) 1992 */
	DRIVER( suzuka8h )	/* (c) 1992 (World) */
	DRIVER( suzuk8hj )	/* (c) 1992 (Japan) */
	DRIVER( sws )		/* (c) 1992 (Japan) */
	DRIVER( sws92 )		/* (c) 1992 (Japan) */
	DRIVER( sws92g )	/* (c) 1992 (Japan) */
	DRIVER( suzuk8h2 )	/* (c) 1993 (World) */
	DRIVER( sws93 )		/* (c) 1993 (Japan) */

	/* Namco FL games */
	DRIVER( speedrcr )	/* (c) 1995 */
	DRIVER( finalapr )	/* (c) 1995 */
	DRIVER( finalapo )	/* (c) 1995 */

	/* Namco NA-1 / NA-2 System games */
	DRIVER( bkrtmaq )	/* (c) 1992 (Japan) */
	DRIVER( cgangpzl )	/* (c) 1992 (US) */
	DRIVER( cgangpzj )	/* (c) 1992 (Japan) */
	DRIVER( exvania )	/* (c) 1992 (Japan) */
	DRIVER( fghtatck )	/* (c) 1992 (US) */
	DRIVER( fa )		/* (c) 1992 (Japan) */
	DRIVER( knckhead )	/* (c) 1992 (World) */
	DRIVER( knckhedj )	/* (c) 1992 (Japan) */
	DRIVER( swcourt )	/* (c) 1992 (Japan) */
	DRIVER( swcourtj )	/* (c) 1992 (Japan) */
	DRIVER( emeralda )	/* (c) 1993 (Japan) */
	DRIVER( emerldaa )	/* (c) 1993 (Japan) */
	DRIVER( numanath )	/* (c) 1993 (World) */
	DRIVER( numanatj )	/* (c) 1993 (Japan) */
	DRIVER( quiztou )	/* (c) 1993 (Japan) */
	DRIVER( tinklpit )	/* (c) 1993 (Japan) */
	DRIVER( xday2 )		/* (c) 1995 (Japan) */

	/* Namco NB-1 / NB-2 System games */
	DRIVER( nebulray )	/* (c) 1994 (World) */
	DRIVER( nebulryj )	/* (c) 1994 (Japan) */
	DRIVER( ptblank )	/* (c) 1994 */
	DRIVER( gunbulet )	/* (c) 1994 (Japan) */
	DRIVER( gslugrsj )	/* (c) 1993 (Japan) */
	DRIVER( gslgr94u )	/* (c) 1994 */
	DRIVER( sws95 )		/* (c) 1995 (Japan) */
	DRIVER( sws96 )		/* (c) 1996 (Japan) */
	DRIVER( sws97 )		/* (c) 1997 (Japan) */
	DRIVER( vshoot )	/* (c) 1994 */
	DRIVER( outfxies )	/* (c) 1994 */
	DRIVER( outfxesj )	/* (c) 1994 (Japan) */
	DRIVER( machbrkr )	/* (c) 1995 (Japan) */

	/* Namco ND-1 games */
	DRIVER( ncv1 )		/* (c) 1995 */
	DRIVER( ncv1j )		/* (c) 1995 (Japan) */
	DRIVER( ncv1j2 )	/* (c) 1995 (Japan) */
	DRIVER( ncv2 )		/* (c) 1996 */
	DRIVER( ncv2j )		/* (c) 1996 (Japan) */

	/* Namco System 21 games */
	DRIVER( winrun )
				/* 1989, Winning Run Suzuka Grand Prix */
	DRIVER( winrun91 )
	DRIVER( solvalou )	/* (c) 1991 (Japan) */
	DRIVER( starblad )	/* (c) 1991 */
	DRIVER( driveyes )	/* (c) 1992? */
				/* 1992, ShimDrive */
	DRIVER( aircombj )	/* (c) 1992 (Japan) */
	DRIVER( aircombu )	/* (c) 1992 (US) */
	DRIVER( cybsled )	/* (c) 1993 */

	/* Namco System 22 games */
	DRIVER( ridgerac )	/* (c) 1993 */
	DRIVER( ridgeraj )	/* (c) 1993 */
	DRIVER( acedrvrw )	/* (c) 1994 */
	DRIVER( alpinerd )	/* (c) 1994 */
	DRIVER( alpinerc )	/* (c) 1994 */
	DRIVER( ridgera2 )	/* (c) 1994 */
	DRIVER( ridger2a )	/* (c) 1994 */
	DRIVER( ridger2b )	/* (c) 1994 */
	DRIVER( raveracw )	/* (c) 1995 */
	DRIVER( raveracj )	/* (c) 1995 */
	DRIVER( raveraja )	/* (c) 1995 */
	DRIVER( airco22b )	/* (c) 1995 */
	DRIVER( alpinr2b )	/* (c) 1995 */
	DRIVER( cybrcomm )	/* (c) 1995 */
	DRIVER( cybrcycc )	/* (c) 1995 */
	DRIVER( timecris )	/* (c) 1995 */
	DRIVER( timecrsa )	/* (c) 1995 */
	DRIVER( victlapw )	/* (c) 1996 */
	DRIVER( alpinesa )	/* (c) 1996 */
	DRIVER( propcycl )	/* (c) 1996 */

	/* Namco System 23 */
	DRIVER( timecrs2 )	/* (c) 1997 */
	DRIVER( gp500 )		/* (c) 1999 */
	DRIVER( finfurl2 )	/* (c) 1999 */
	DRIVER( finfrl2j )	/* (c) 1999 */


	/* Universal games */
	DRIVER( cosmicg )	/* 7907 (c) 1979 */
	DRIVER( cosmica )	/* 7910 (c) [1979] */
	DRIVER( cosmica2 )	/* 7910 (c) 1979 */
	DRIVER( panic )		/* (c) 1980 */
	DRIVER( panic2 )	/* (c) 1980 */
	DRIVER( panic3 )	/* (c) 1980 */
	DRIVER( panich )	/* (c) 1980 */
	DRIVER( panicger )	/* (c) 1980 */
	DRIVER( zerohour )	/* 8011 (c) Universal */
	DRIVER( redclash )	/* (c) 1981 Tehkan */
	DRIVER( redclsha )	/* (c) 1981 Tehkan */
	DRIVER( redclask )	/* (c) Kaneko (bootleg?) */
	DRIVER( magspot )	/* ???? (c) [1980] */
	DRIVER( magspot2 )	/* 8013 (c) [1980] */
	DRIVER( devzone )	/* 8022 (c) [1980] */
	DRIVER( devzone2 )	/* 8022 (c) [1980] */
	DRIVER( nomnlnd )	/* (c) [1980?] */
	DRIVER( nomnlndg )	/* (c) [1980?] + Gottlieb */
	DRIVER( cheekyms )	/* (c) [1980?] */
	DRIVER( ladybug )	/* (c) 1981 */
	DRIVER( ladybugb )	/* bootleg */
	DRIVER( ladybgb2 )	/* bootleg */
	DRIVER( snapjack )	/* (c) */
	DRIVER( sraider )	/* (c) 1982 Universal */
	DRIVER( cavenger )	/* (c) 1981 */
	DRIVER( dorodon )	/* Falcon */
	DRIVER( dorodon2 )	/* Falcon */
	DRIVER( mrdo )		/* (c) 1982 */
	DRIVER( mrdoy )		/* (c) 1982 */
	DRIVER( mrdot )		/* (c) 1982 + Taito license */
	DRIVER( mrdofix )	/* (c) 1982 + Taito license */
	DRIVER( mrlo )		/* bootleg */
	DRIVER( mrdu )		/* bootleg */
	DRIVER( yankeedo )	/* bootleg */
	DRIVER( docastle )	/* (c) 1983 */
	DRIVER( docastl2 )	/* (c) 1983 */
	DRIVER( docastlo )	/* (c) 1983 */
	DRIVER( douni )		/* (c) 1983 */
	DRIVER( dorunrun )	/* (c) 1984 */
	DRIVER( dorunru2 )	/* (c) 1984 */
	DRIVER( dorunruc )	/* (c) 1984 */
	DRIVER( dorunrca )	/* (c) 1984 */
	DRIVER( spiero )	/* (c) 1987 */
	DRIVER( dowild )	/* (c) 1984 */
	DRIVER( jjack )		/* (c) 1984 */
	DRIVER( kickridr )	/* (c) 1984 */
	DRIVER( idsoccer )	/* (c) 1985 */

	/* Nintendo games */
	DRIVER( spacefev )	/* (c) 1979 Nintendo */
	DRIVER( spacefva )	/* (c) 1979 Nintendo */
	DRIVER( highsplt )	/* (c) 1979 Nintendo */
	DRIVER( highspla )	/* (c) 1979 Nintendo */
	DRIVER( spacelnc )	/* (c) 1979 Nintendo */
	DRIVER( sheriff )	/* (c) 1979 Nintendo */
	DRIVER( bandido )	/* (c) 1980 Exidy */
	DRIVER( helifire )	/* (c) 1980 Nintendo */
	DRIVER( helifira )	/* (c) 1980 Nintendo */
	DRIVER( radarscp )	/* (c) 1980 Nintendo */
	DRIVER( dkong )		/* (c) 1981 Nintendo of America */
	DRIVER( dkongo )	/* (c) 1981 Nintendo */
	DRIVER( dkongpe )	/* (c) 1981,2013 Nintendo */
	DRIVER( dkongjp )	/* (c) 1981 Nintendo */
	DRIVER( dkongjo )	/* (c) 1981 Nintendo */
	DRIVER( dkongjo1 )	/* (c) 1981 Nintendo */
	DRIVER( dkongjr )	/* (c) 1982 Nintendo of America */
	DRIVER( dkongjrj )	/* (c) 1982 Nintendo */
	DRIVER( dkngjnrj )	/* (c) 1982 Nintendo */
	DRIVER( dkongjrb )	/* bootleg */
	DRIVER( dkngjnrb )	/* (c) 1982 Nintendo of America */
	DRIVER( dkong3 )	/* (c) 1983 Nintendo of America */
	DRIVER( dkong3j )	/* (c) 1983 Nintendo */
	DRIVER( dkong3b )	/* bootleg */
	DRIVER( mario )		/* (c) 1983 Nintendo of America */
	DRIVER( mariojp )	/* (c) 1983 Nintendo */
	DRIVER( masao )		/* bootleg */
	DRIVER( pestplce )	/* bootleg on donkey kong hw */
	DRIVER( spclforc )
	DRIVER( spcfrcii )
	DRIVER( 8ballact )
	DRIVER( 8ballat2 )
	DRIVER( shootgal )
	DRIVER( drakton )
	DRIVER( strtheat )
	DRIVER( hunchbkd )	/* (c) 1983 Century */
	DRIVER( sbdk )		/* (c) 1984 Century */
	DRIVER( herbiedk )	/* (c) 1984 CVS */
	DRIVER( herodk )	/* (c) 1984 Seatongrove + Crown license */
	DRIVER( herodku )	/* (c) 1984 Seatongrove + Crown license */
	DRIVER( skyskipr )	/* (c) 1981 */
	DRIVER( popeye )	/* (c) 1982 */
	DRIVER( popeyeu )	/* (c) 1982 */
	DRIVER( popeyef )	/* (c) 1982 */
	DRIVER( popeyebl )	/* bootleg */
	DRIVER( punchout )	/* (c) 1984 */
	DRIVER( spnchout )	/* (c) 1984 */
	DRIVER( spnchotj )	/* (c) 1984 (Japan) */
	DRIVER( armwrest )	/* (c) 1985 */

	/* Nintendo Playchoice 10 games */
	DRIVER( playch10 )
	DRIVER( pc_tenis )	/* (c) 1983 Nintendo */
	DRIVER( pc_mario )	/* (c) 1983 Nintendo */
	DRIVER( pc_bball )	/* (c) 1984 Nintendo of America */
	DRIVER( pc_bfght )	/* (c) 1984 Nintendo */
	DRIVER( pc_ebike )	/* (c) 1984 Nintendo */
	DRIVER( pc_golf )	/* (c) 1984 Nintendo */
	DRIVER( pc_kngfu )	/* (c) 1984 Irem (Nintendo license) */
	DRIVER( pc_1942 )	/* (c) 1985 Capcom */
	DRIVER( pc_smb )	/* (c) 1985 Nintendo */
	DRIVER( pc_vball )	/* (c) 1986 Nintendo */
	DRIVER( pc_duckh )	/* (c) 1984 Nintendo */
	DRIVER( pc_hgaly )	/* (c) 1984 Nintendo */
	DRIVER( pc_wgnmn )	/* (c) 1984 Nintendo */
	DRIVER( pc_grdus )	/* (c) 1986 Konami */
	DRIVER( pc_grdue )	/* (c) 1986 Konami */
	DRIVER( pc_tkfld )	/* (c) 1987 Konami (Nintendo of America license) */
	DRIVER( pc_pwrst )	/* (c) 1986 Nintendo */
	DRIVER( pc_trjan )	/* (c) 1986 Capcom USA (Nintendo of America license) */
	DRIVER( pc_cvnia )	/* (c) 1987 Konami (Nintendo of America license) */
	DRIVER( pc_dbldr )	/* (c) 1987 Konami (Nintendo of America license) */
	DRIVER( pc_rnatk )	/* (c) 1987 Konami (Nintendo of America license) */
	DRIVER( pc_rygar )	/* (c) 1987 Tecmo (Nintendo of America license) */
	DRIVER( pc_cntra )	/* (c) 1988 Konami (Nintendo of America license) */
	DRIVER( pc_goons )	/* (c) 1986 Konami */
	DRIVER( pc_mtoid )	/* (c) 1986 Nintendo */
	DRIVER( pc_radrc )	/* (c) 1987 Square */
	DRIVER( pc_miket )	/* (c) 1987 Nintendo */
	DRIVER( pc_rcpam )	/* (c) 1987 Rare */
	DRIVER( pc_ngaid )	/* (c) 1989 Tecmo (Nintendo of America license) */
	DRIVER( pc_tmnt )	/* (c) 1989 Konami (Nintendo of America license) */
	DRIVER( pc_ftqst )	/* (c) 1989 Sunsoft (Nintendo of America license) */
	DRIVER( pc_bstar )	/* (c) 1989 SNK (Nintendo of America license) */
	DRIVER( pc_tbowl )	/* (c) 1989 Tecmo (Nintendo of America license) */
	DRIVER( pc_drmro )	/* (c) 1990 Nintendo */
	DRIVER( pc_ynoid )	/* (c) 1990 Capcom USA (Nintendo of America license) */
	DRIVER( pc_rrngr )	/* (c) Capcom USA (Nintendo of America license) */
	DRIVER( pc_ddrgn )
	DRIVER( pc_gntlt )	/* (c) 1985 Atari/Tengen (Nintendo of America license) */
	DRIVER( pc_smb2 )	/* (c) 1988 Nintendo */
	DRIVER( pc_smb3 )	/* (c) 1988 Nintendo */
	DRIVER( pc_mman3 )	/* (c) 1990 Capcom USA (Nintendo of America license) */
	DRIVER( pc_radr2 )	/* (c) 1990 Square (Nintendo of America license) */
	DRIVER( pc_suprc )	/* (c) 1990 Konami (Nintendo of America license) */
	DRIVER( pc_tmnt2 )	/* (c) 1990 Konami (Nintendo of America license) */
	DRIVER( pc_wcup )	/* (c) 1990 Technos (Nintendo license) */
	DRIVER( pc_ngai2 )	/* (c) 1990 Tecmo (Nintendo of America license) */
	DRIVER( pc_ngai3 )	/* (c) 1991 Tecmo (Nintendo of America license) */
	DRIVER( pc_pwbld )	/* (c) 1991 Taito (Nintendo of America license) */
	DRIVER( pc_rkats )	/* (c) 1991 Atlus (Nintendo of America license) */
	DRIVER( pc_pinbt )	/* (c) 1988 Rare (Nintendo of America license) */
	DRIVER( pc_cshwk )	/* (c) 1989 Rare (Nintendo of America license) */
	DRIVER( pc_sjetm )	/* (c) 1990 Rare */
	DRIVER( pc_moglf )	/* (c) 1991 Nintendo */

	/* Nintendo VS games */
	DRIVER( btlecity )	/* (c) 1985 Namco */
	DRIVER( starlstr )	/* (c) 1985 Namco */
	DRIVER( cstlevna )	/* (c) 1987 Konami */
	DRIVER( cluclu )	/* (c) 1984 Nintendo */
	DRIVER( drmario )	/* (c) 1990 Nintendo */
	DRIVER( duckhunt )	/* (c) 1985 Nintendo */
	DRIVER( excitebk )	/* (c) 1984 Nintendo */
	DRIVER( excitbkj )	/* (c) 1984 Nintendo */
	DRIVER( goonies )	/* (c) 1986 Konami */
	DRIVER( hogalley )	/* (c) 1985 Nintendo */
	DRIVER( iceclimb )	/* (c) 1984 Nintendo */
	DRIVER( iceclmbj )	/* (c) 1984 Nintendo */
	DRIVER( ladygolf )	/* (c) 1984 Nintendo */
	DRIVER( machridr )	/* (c) 1985 Nintendo */
	DRIVER( machridj )	/* (c) 1985 Nintendo */
	DRIVER( rbibb )		/* (c) 1987 Namco */
	DRIVER( rbibba )	/* (c) 1987 Namco */
	DRIVER( suprmrio )	/* (c) 1986 Nintendo */
	DRIVER( vsskykid )	/* (c) 1986 Namco */
	DRIVER( tkoboxng )	/* (c) 1987 Data East */
	DRIVER( smgolf )	/* (c) 1984 Nintendo */
	DRIVER( smgolfj )	/* (c) 1984 Nintendo */
	DRIVER( vspinbal )	/* (c) 1984 Nintendo */
	DRIVER( vspinblj )	/* (c) 1984 Nintendo */
	DRIVER( vsslalom )	/* (c) 1986 Nintendo */
	DRIVER( vssoccer )	/* (c) 1985 Nintendo */
	DRIVER( vsgradus )	/* (c) 1986 Konami */
	DRIVER( platoon )	/* (c) 1987 Ocean */
	DRIVER( vstetris )	/* (c) 1988 Atari */
	DRIVER( mightybj )	/* (c) 1986 Tecmo */
	DRIVER( jajamaru )	/* (c) 1985 Jaleco */
	DRIVER( topgun )	/* (c) 1987 Konami */
	DRIVER( bnglngby )	/* (c) 1985 Nintendo / Broderbund Software Inc. */
	DRIVER( vstennis )	/* (c) 1984 Nintendo */
	DRIVER( vstennij )	/* (c) 1984 Nintendo */
	DRIVER( wrecking )	/* (c) 1984 Nintendo */
	DRIVER( balonfgt )	/* (c) 1984 Nintendo */
	DRIVER( vsmahjng )	/* (c) 1984 Nintendo */
	DRIVER( vsbball )	/* (c) 1984 Nintendo */
	DRIVER( vsbballj )	/* (c) 1984 Nintendo */
	DRIVER( vsbbalja )	/* (c) 1984 Nintendo */
	DRIVER( iceclmrj )	/* (c) 1984 Nintendo */
	DRIVER( vsgshoe )	/* (c) 1986 Nintendo */
	DRIVER( supxevs )
	DRIVER( vsfdf )
	DRIVER( smgolfb )
	DRIVER( vsbbaljb )

	/* Nintendo Super System Games */
	DRIVER( nss )
	DRIVER( nss_ssoc )
	DRIVER( nss_actr )
	DRIVER( nss_con3 )
	DRIVER( nss_adam )
	DRIVER( nss_aten )
	DRIVER( nss_rob3 )
	DRIVER( nss_ncaa )
	DRIVER( nss_skin )
	DRIVER( nss_lwep )
	DRIVER( nss_smw )
	DRIVER( nss_fzer )
	DRIVER( nss_sten )

	/* Midway 8080 b/w games */
	DRIVER( seawolf )	/* 596 [1976] */
	DRIVER( gunfight )	/* 597 [1975] */
	/* 603 - Top Gun [1976] */
	DRIVER( tornbase )	/* 605 [1976] */
	DRIVER( 280zzzap )	/* 610 [1976] */
	DRIVER( maze )		/* 611 [1976] */
	DRIVER( boothill )	/* 612 [1977] */
	DRIVER( checkmat )	/* 615 [1977] */
	DRIVER( desertgu )	/* 618 [1977] */
	DRIVER( dplay )		/* 619 [1977] */
	DRIVER( lagunar )	/* 622 [1977] */
	DRIVER( gmissile )	/* 623 [1977] */
	DRIVER( m4 )		/* 626 [1977] */
	DRIVER( clowns )	/* 630 [1978] */
	DRIVER( clowns1 )	/* 630 [1978] */
				/* 640 - Space Walk [1978] */
	DRIVER( einnings )	/* 642 [1978] Midway */
	DRIVER( shuffle )	/* 643 [1978] */
	DRIVER( dogpatch )	/* 644 [1977] */
	DRIVER( spcenctr )	/* 645 (c) 1980 Midway */
	DRIVER( phantom2 )	/* 652 [1979] */
	DRIVER( bowler )	/* 730 [1978] Midway */
	DRIVER( invaders )	/* 739 [1979] */
	DRIVER( blueshrk )	/* 742 [1978] */
	DRIVER( invad2ct )	/* 851 (c) 1980 Midway */
	DRIVER( invadpt2 )	/* 852 [1980] Taito */
	DRIVER( invaddlx )	/* 852 [1980] Midway */
	DRIVER( moonbase )	/* Zeta - Nichibutsu */
				/* 870 - Space Invaders Deluxe cocktail */
	DRIVER( searthin )
	DRIVER( searthia )
	DRIVER( invadrmr )
	DRIVER( spaceatt )
	DRIVER( spaceat2 )
	DRIVER( sinvzen )
	DRIVER( superinv )
	DRIVER( invasion )
	DRIVER( sstrangr )
	DRIVER( sstrngr2 )
	DRIVER( sinvemag )
	DRIVER( jspecter )
	DRIVER( jspectr2 )
	DRIVER( invrvnge )
	DRIVER( invrvnga )
	DRIVER( galxwars )
	DRIVER( galxwar2 )
	DRIVER( galxwart )
	DRIVER( starw )
	DRIVER( lrescue )	/* LR  (c) 1979 Taito */
	DRIVER( grescue )	/* bootleg? */
	DRIVER( desterth )	/* bootleg */
	DRIVER( cosmicmo )	/* Universal */
	DRIVER( cosmicm2 )	/* Universal */
	DRIVER( rollingc )	/* Nichibutsu */
	DRIVER( ozmawars )	/* Shin Nihon Kikaku (SNK) */
	DRIVER( ozmawar2 )	/* Shin Nihon Kikaku (SNK) */
	DRIVER( solfight )	/* bootleg */
	DRIVER( spaceph )	/* Zilec Games */
	DRIVER( schaser )	/* RT  Taito */
	DRIVER( schasrcv )	/* RT  Taito */
	DRIVER( lupin3 )	/* LP  (c) 1980 Taito */
	DRIVER( spclaser )
	DRIVER( laser )
	DRIVER( spcewarl )
	DRIVER( polaris )	/* PS  (c) 1980 Taito */
	DRIVER( polarisa )	/* PS  (c) 1980 Taito */
	DRIVER( ballbomb )	/* TN  (c) 1980 Taito */
	DRIVER( indianbt )	/* (c) 1980 Taito */
	DRIVER( m79amb )
	DRIVER( alieninv )
	DRIVER( tst_invd )
	DRIVER( sitv )
	DRIVER( sicv )
	DRIVER( sisv )
	DRIVER( sisv2 )
	DRIVER( spacewr3 )
	DRIVER( invaderl )
	DRIVER( invader4 )
	DRIVER( yosakdon )
	DRIVER( yosakdoa )
	DRIVER( spceking )
	DRIVER( spcewars )
	DRIVER( cosmo )		/* TDS+Mints */

	DRIVER( sspeedr )	/* 1979 Midway */

	/* Similar but with a M6800 instead of an 8080 */
	DRIVER( sflush ) /* (c)1979 Taito */

	/* Meadows S2650 games */
	DRIVER( lazercmd )	/* [1976?] */
	DRIVER( bbonk )		/* [1976?] */
	DRIVER( deadeye )	/* [1978?] */
	DRIVER( gypsyjug )	/* [1978?] */
	DRIVER( minferno )	/* [1978?] */
	DRIVER( medlanes )	/* [1977?] */

	/* CVS games */
	DRIVER( cvs )
	DRIVER( cosmos )	/* (c) 1981 Century */
	DRIVER( darkwar )	/* (c) 1981 Century */
	DRIVER( spacefrt )	/* (c) 1981 Century */
	DRIVER( 8ball )		/* (c) 1982 Century */
	DRIVER( 8ball1 )	/* (c) 1982 Century */
	DRIVER( logger )	/* (c) 1982 Century */
	DRIVER( dazzler )	/* (c) 1982 Century */
	DRIVER( diggerc )	/* (c) 1982 Century */
	DRIVER( wallst )	/* (c) 1982 Century */
	DRIVER( radarzon )	/* (c) 1982 Century */
	DRIVER( radarzn1 )	/* (c) 1982 Century */
	DRIVER( radarznt )	/* (c) 1982 Tuni Electro Service */
	DRIVER( outline )	/* (c) 1982 Century */
	DRIVER( goldbug )	/* (c) 1982 Century */
	DRIVER( heartatk )	/* (c) 1983 Century Electronics */
	DRIVER( hunchbak )	/* (c) 1983 Century */
	DRIVER( superbik )	/* (c) 1983 Century */
	DRIVER( raiders )	/* (c) 1983 Century Electronics UK */
	DRIVER( hero )		/* (c) 1983 Seatongrove (c) 1984 CVS */
	DRIVER( huncholy )	/* (c) 1984 Seatongrove (c) CVS */

	/* Zaccaria Hardware (similar to CVS) */
	DRIVER( quasar )	/* (c) 1980 Zelco Games Italy */
	DRIVER( quasara )	/* (c) 1980 Zelco Games Italy */

	/* Midway "Astrocade" games */
	DRIVER( seawolf2 )
	DRIVER( spacezap )	/* (c) 1980 */
	DRIVER( ebases )
	DRIVER( wow )		/* (c) 1980 */
	DRIVER( gorf )		/* (c) 1981 */
	DRIVER( gorfpgm1 )	/* (c) 1981 */
	DRIVER( robby )		/* (c) 1981 Bally Midway */
	DRIVER( demndrgn )	/* (c) 1982 Bally Midway */
	DRIVER( profpac )	/* (c) 1983 Bally Midway */

	/* Bally Midway MCR games */
	/* MCR1 */
	DRIVER( solarfox )	/* (c) 1981 */
	DRIVER( kick )		/* (c) 1981 */
	DRIVER( kicka )		/* bootleg? */
	/* MCR2 */
	DRIVER( shollow )	/* (c) 1981 */
	DRIVER( shollow2 )	/* (c) 1981 */
	DRIVER( tron )		/* (c) 1982 */
	DRIVER( tron2 )		/* (c) 1982 */
	DRIVER( tron3 )		/* (c) 1982 */
	DRIVER( tron4 )		/* (c) 1982 */
	DRIVER( kroozr )	/* (c) 1982 */
	DRIVER( domino )	/* (c) 1982 */
	DRIVER( wacko )		/* (c) 1982 */
	DRIVER( twotiger )	/* (c) 1984 */
	DRIVER( twotigrc )	/* (c) 1984 */
	/* MCR2 + MCR3 sprites */
	DRIVER( journey )	/* (c) 1983 */
	/* MCR3 */
	DRIVER( tapper )	/* (c) 1983 */
	DRIVER( tappera )	/* (c) 1983 */
	DRIVER( sutapper )	/* (c) 1983 */
	DRIVER( rbtapper )	/* (c) 1984 */
	DRIVER( timber )	/* (c) 1984 */
	DRIVER( dotron )	/* (c) 1983 */
	DRIVER( dotrona )	/* (c) 1983 */
	DRIVER( dotrone )	/* (c) 1983 */
	DRIVER( nflfoot )	/* (c) 1983 + laserdisk */
	DRIVER( demoderb )	/* (c) 1984 */
	DRIVER( demoderm )	/* (c) 1984 */
	DRIVER( sarge )		/* (c) 1985 */
	DRIVER( rampage )	/* (c) 1986 */
	DRIVER( rampage2 )	/* (c) 1986 */
	DRIVER( powerdrv )	/* (c) 1986 */
	DRIVER( stargrds )	/* (c) 1987 */
	DRIVER( maxrpm )	/* (c) 1986 */
	DRIVER( spyhunt )	/* (c) 1983 */
	DRIVER( turbotag )	/* (c) 1985 */
	DRIVER( crater )	/* (c) 1984 */
	/* MCR 68000 */
	DRIVER( zwackery )	/* (c) 1984 */
	DRIVER( xenophob )	/* (c) 1987 */
	DRIVER( spyhunt2 )	/* (c) 1987 */
	DRIVER( spyhnt2a )	/* (c) 1987 */
	DRIVER( blasted )	/* (c) 1988 */
	DRIVER( archrivl )	/* (c) 1989 */
	DRIVER( archriv2 )	/* (c) 1989 */
	DRIVER( trisport )	/* (c) 1989 */
	DRIVER( pigskin )	/* (c) 1990 */

	/* Bally / Sente games */
	DRIVER( sentetst )
	DRIVER( cshift )	/* (c) 1984 */
	DRIVER( gghost )	/* (c) 1984 */
	DRIVER( hattrick )	/* (c) 1984 */
	DRIVER( otwalls )	/* (c) 1984 */
	DRIVER( snakepit )	/* (c) 1984 */
	DRIVER( snakjack )	/* (c) 1984 */
	DRIVER( stocker )	/* (c) 1984 */
	DRIVER( triviag1 )	/* (c) 1984 */
	DRIVER( triviag2 )	/* (c) 1984 */
	DRIVER( triviasp )	/* (c) 1984 */
	DRIVER( triviayp )	/* (c) 1984 */
	DRIVER( triviabb )	/* (c) 1984 */
	DRIVER( triviaes )	/* (c) 1984 */
	DRIVER( gimeabrk )	/* (c) 1985 */
	DRIVER( minigolf )	/* (c) 1985 */
	DRIVER( minigol2 )	/* (c) 1985 */
	DRIVER( toggle )	/* (c) 1985 */
	DRIVER( nametune )	/* (c) 1986 */
	DRIVER( nstocker )	/* (c) 1986 */
	DRIVER( sfootbal )	/* (c) 1986 */
	DRIVER( spiker )	/* (c) 1986 */
	DRIVER( stompin )	/* (c) 1986 */
	DRIVER( rescraid )	/* (c) 1987 */
	DRIVER( rescrdsa )	/* (c) 1987 */
	DRIVER( grudge )
	DRIVER( shrike )	/* (c) 1987 */
	DRIVER( gridlee )	/* [1983 Videa] prototype - no copyright notice */

	/* Irem games */
	/* trivia: IREM means "International Rental Electronics Machines" */
	DRIVER( andromed )
	DRIVER( ipminvad )	/* M10 no copyright notice */
	DRIVER( skychut )	/* Irem [1980] */
	DRIVER( spacbeam )	/* M15 no copyright notice */
	DRIVER( greenber )	/* Irem */
	DRIVER( headoni )	/* Irem no copyright notice */

	DRIVER( redalert )	/* (c) 1981 + "GDI presents" */
	DRIVER( demoneye )	/* (c) 1981 */
	DRIVER( olibochu )	/* M47 (c) 1981 + "GDI presents" */
	DRIVER( mpatrol )	/* M52 (c) 1982 */
	DRIVER( mpatrolw )	/* M52 (c) 1982 + Williams license */
	DRIVER( troangel )	/* (c) 1983 */
	DRIVER( yard )		/* (c) 1983 */
	DRIVER( vsyard )	/* (c) 1983/1984 */
	DRIVER( vsyard2 )	/* (c) 1983/1984 */
	DRIVER( travrusa )	/* (c) 1983 */
	DRIVER( motorace )	/* (c) 1983 Williams license */
	DRIVER( shtrider )	/* (c) 1984 Seibu Kaihatsu */
	DRIVER( shtridra )	/* (c) 1984 Seibu Kaihatsu */
	DRIVER( wilytowr )	/* M63 (c) 1984 */
	DRIVER( atomboy )	/* M63 (c) 1985 Irem + Memetron license */
	DRIVER( fghtbskt )	/* (c) 1984 Paradise Co. Ltd. */
	/* M62 */
	DRIVER( kungfum )	/* (c) 1984 */
	DRIVER( kungfud )	/* (c) 1984 + Data East license */
	DRIVER( spartanx )	/* (c) 1984 (Japan) */
	DRIVER( kungfub )	/* bootleg */
	DRIVER( kungfub2 )	/* bootleg */
	DRIVER( battroad )	/* (c) 1984 */
	DRIVER( ldrun )		/* (c) 1984 licensed from Broderbund */
	DRIVER( ldruna )	/* (c) 1984 licensed from Broderbund */
	DRIVER( ldrun2 )	/* (c) 1984 licensed from Broderbund */
	DRIVER( ldrun3 )	/* (c) 1985 licensed from Broderbund */
	DRIVER( ldrun3jp )	/* (c) 1985 licensed from Broderbund */
	DRIVER( ldrun4 )	/* (c) 1986 licensed from Broderbund */
	DRIVER( lotlot )	/* (c) 1985 licensed from Tokuma Shoten */
	DRIVER( kidniki )	/* (c) 1986 + Data East USA license */
	DRIVER( kidnikiu )	/* (c) 1986 + Data East USA license */
	DRIVER( yanchamr )	/* (c) 1986 (Japan) */
	DRIVER( lithero )	/* bootleg */
	DRIVER( spelunkr )	/* (c) 1985 licensed from Broderbund */
	DRIVER( spelnkrj )	/* (c) 1985 licensed from Broderbund */
	DRIVER( spelunk2 )	/* (c) 1986 licensed from Broderbund */
	DRIVER( horizon )	/* (c) 1985 */
	DRIVER( youjyudn )	/* (c) 1986 (Japan) */

	DRIVER( vigilant )	/* (c) 1988 (World) */
	DRIVER( vigilntu )	/* (c) 1988 (US) */
	DRIVER( vigilntj )	/* (c) 1988 (Japan) */
	DRIVER( kikcubic )	/* (c) 1988 (Japan) */
	DRIVER( kikcubib )	/* bootleg */
	DRIVER( buccanrs )	/* (c) 1989 Duintronic */
	DRIVER( buccanra )	/* (c) 1989 Duintronic */
	/* M72 (and derivatives) */
	DRIVER( rtype )		/* (c) 1987 (Japan) */
	DRIVER( rtypepj )	/* (c) 1987 (Japan) */
	DRIVER( rtypeu )	/* (c) 1987 + Nintendo USA license (US) */
	DRIVER( bchopper )	/* (c) 1987 */
	DRIVER( mrheli )	/* (c) 1987 (Japan) */
	DRIVER( nspirit )	/* (c) 1988 */
	DRIVER( nspiritj )	/* (c) 1988 (Japan) */
	DRIVER( imgfight )	/* (c) 1988 (Japan) */
	DRIVER( loht )		/* (c) 1989 */
	DRIVER( xmultipl )	/* (c) 1989 (Japan) */
	DRIVER( dbreed )	/* (c) 1989 */
	DRIVER( dbreed72 )	/* (c) 1989 */
	DRIVER( rtype2 )	/* (c) 1989 */
	DRIVER( rtype2j )	/* (c) 1989 (Japan) */
	DRIVER( majtitle )	/* (c) 1990 (Japan) */
	DRIVER( hharry )	/* (c) 1990 (World) */
	DRIVER( hharryu )	/* (c) 1990 Irem America (US) */
	DRIVER( dkgensan )	/* (c) 1990 (Japan) */
	DRIVER( dkgenm72 )	/* (c) 1990 (Japan) */
	DRIVER( poundfor )	/* (c) 1990 (World) */
	DRIVER( poundfou )	/* (c) 1990 Irem America (US) */
	DRIVER( airduel )	/* (c) 1990 (Japan) */
	DRIVER( cosmccop )	/* (c) 1991 (World) */
	DRIVER( gallop )	/* (c) 1991 (Japan) */
	DRIVER( kengo )		/* (c) 1991 */
	/* not M72, but same sound hardware */
	DRIVER( sichuan2 )	/* (c) 1989 Tamtex */
	DRIVER( sichuana )	/* (c) 1989 Tamtex */
	DRIVER( shisen )	/* (c) 1989 Tamtex */
	DRIVER( matchit )	/* (c) 1989 Tamtex */
	/* M90 */
	DRIVER( hasamu )	/* (c) 1991 Irem (Japan) */
	DRIVER( dynablst )	/* (c) 1991 Irem (World) */
	DRIVER( dynablsb )	/* bootleg */
	DRIVER( bombrman )	/* (c) 1991 Irem (Japan) */
	DRIVER( atompunk )	/* (c) 1991 Irem America (licensed from Hudson Soft) */
	/* M97 */
	DRIVER( bbmanw )	/* (c) 1992 Irem (World) */
	DRIVER( bbmanwj )	/* (c) 1992 Irem (Japan) */
	DRIVER( bomblord )	/* bootleg */
	DRIVER( newapunk )	/* (c) 1992 Irem America (US) */
	DRIVER( quizf1 )	/* (c) 1992 Irem (Japan) */
	DRIVER( riskchal )	/* (c) 1993 Irem */
	DRIVER( gussun )	/* (c) 1993 Irem (Japan) */
	DRIVER( matchit2 )	/* (c) 1993 Tamtex */
	DRIVER( shisen2 )	/* (c) 1993 Tamtex */
	/* M92 */
	DRIVER( gunforce )	/* (c) 1991 Irem (World) */
	DRIVER( gunforcu )	/* (c) 1991 Irem America (US) */
	DRIVER( gunforcj )	/* (c) 1991 Irem (Japan) */
	DRIVER( bmaster )	/* (c) 1991 Irem */
	DRIVER( lethalth )	/* (c) 1991 Irem (World) */
	DRIVER( thndblst )	/* (c) 1991 Irem (Japan) */
	DRIVER( uccops )	/* (c) 1992 Irem (World) */
	DRIVER( uccopsar )	/* (c) 1992 Irem (Alpha Renewal Version) */
	DRIVER( uccopsj )	/* (c) 1992 Irem (Japan) */
	DRIVER( mysticri )	/* (c) 1992 Irem (World) */
	DRIVER( gunhohki )	/* (c) 1992 Irem (Japan) */
	DRIVER( majtitl2 )	/* (c) 1992 Irem (World) */
	DRIVER( skingame )	/* (c) 1992 Irem America (US) */
	DRIVER( skingam2 )	/* (c) 1992 Irem America (US) */
	DRIVER( hook )		/* (c) 1992 Irem (World) */
	DRIVER( hooku )		/* (c) 1992 Irem America (US) */
	DRIVER( hookj )		/* (c) 1992 Irem (Japan) */
	DRIVER( rtypeleo )	/* (c) 1992 Irem (World) */
	DRIVER( rtypelej )	/* (c) 1992 Irem (Japan) */
	DRIVER( inthunt )	/* (c) 1993 Irem (World) */
	DRIVER( inthuntu )	/* (c) 1993 Irem (US) */
	DRIVER( kaiteids )	/* (c) 1993 Irem (Japan) */
	DRIVER( nbbatman )	/* (c) 1993 Irem America (US) */
	DRIVER( leaguemn )	/* (c) 1993 Irem (Japan) */
	DRIVER( ssoldier )	/* (c) 1993 Irem America (US) */
	DRIVER( psoldier )	/* (c) 1993 Irem (Japan) */
	DRIVER( dsccr94j )	/* (c) 1994 Irem (Japan) */
	DRIVER( gunforc2 )	/* (c) 1994 Irem */
	DRIVER( geostorm )	/* (c) 1994 Irem (Japan) */
	/* M107 */
	DRIVER( firebarr )	/* (c) 1993 Irem (Japan) */
	DRIVER( dsoccr94 )	/* (c) 1994 Irem (Data East Corporation license) */
	DRIVER( wpksoc )	/* (c) 1995 Jaleco */

	/* Gottlieb/Mylstar games (Gottlieb became Mylstar in 1983) */
	DRIVER( reactor )	/* GV-100 (c) 1982 Gottlieb */
	DRIVER( mplanets )	/* GV-102 (c) 1983 Gottlieb */
	DRIVER( mplanuk )	/* GV-102 (c) 1983 Gottlieb */
	DRIVER( qbert )		/* GV-103 (c) 1982 Gottlieb */
	DRIVER( qberta )	/* GV-103 (c) 1982 Gottlieb */
	DRIVER( qbertjp )	/* GV-103 (c) 1982 Gottlieb + Konami license */
	DRIVER( myqbert )	/* GV-103?(c) 1982 Gottlieb */
	DRIVER( qberttst )	/* GV-103 (c) 1982 Gottlieb */
	DRIVER( qbtrktst )	/* GV-103 (c) 1982 Gottlieb */
	DRIVER( insector )	/* GV-??? (c) 1982 Gottlieb - never released */
	DRIVER( tylz )		/* GV-??? (c) 1982 Gottlieb - never released */
	DRIVER( argusg )	/* GV-??? (c) 1982 Gottlieb - never released */
	DRIVER( krull )		/* GV-105 (c) 1983 Gottlieb */
	DRIVER( kngtmare )	/* GV-??? (c) 1983 Gottlieb - never released */
	DRIVER( sqbert )	/* GV-??? (c) 1983 Mylstar - never released */
	DRIVER( mach3 )		/* GV-109 (c) 1983 Mylstar */
	DRIVER( usvsthem )	/* GV-??? (c) 198? Mylstar */
	DRIVER( 3stooges )	/* GV-113 (c) 1984 Mylstar */
	DRIVER( qbertqub )	/* GV-119 (c) 1983 Mylstar */
	DRIVER( screwloo )	/* GV-123 (c) 1983 Mylstar - never released */
	DRIVER( curvebal )	/* GV-134 (c) 1984 Mylstar */
	DRIVER( vidvince )	/* GV-??? (c) 1984 Mylstar - never released  */
	DRIVER( wizwarz )	/* GV-??? (c) 1984 Mylstar - never released */

	/* Taito "Qix hardware" games */
	DRIVER( qix )		/* LK  (c) 1981 Taito America Corporation */
	DRIVER( qixa )		/* LK  (c) 1981 Taito America Corporation */
	DRIVER( qixb )		/* LK  (c) 1981 Taito America Corporation */
	DRIVER( qix2 )		/* ??  (c) 1981 Taito America Corporation */
	DRIVER( sdungeon )	/* SD  (c) 1981 Taito America Corporation */
	DRIVER( elecyoyo )	/* YY  (c) 1982 Taito America Corporation */
	DRIVER( elecyoy2 )	/* YY  (c) 1982 Taito America Corporation */
	DRIVER( kram )		/* KS  (c) 1982 Taito America Corporation */
	DRIVER( kram2 )		/* KS  (c) 1982 Taito America Corporation */
	DRIVER( kram3 )
	DRIVER( zookeep )	/* ZA  (c) 1982 Taito America Corporation */
	DRIVER( zookeep2 )	/* ZA  (c) 1982 Taito America Corporation */
	DRIVER( zookeep3 )	/* ZA  (c) 1982 Taito America Corporation */
	DRIVER( slither )	/* (c) 1982 Century II */
	DRIVER( slithera )	/* (c) 1982 Century II */
	DRIVER( complexx )	/* CX  (c) 1984 Taito America Corporation */

	/* Taito SJ System games */
	DRIVER( spaceskr )	/* EB  (c) 1981 Taito Corporation */
	DRIVER( spacecr )	/* CG  (c) 1981 Taito Corporation */
	DRIVER( junglek )	/* KN  (c) 1982 Taito Corporation */
	DRIVER( junglkj2 )	/* KN  (c) 1982 Taito Corporation */
	DRIVER( jungleh )	/* KN  (c) 1982 Taito America Corporation */
	DRIVER( junglhbr )	/* KN  (c) 1982 Taito do Brasil */
	DRIVER( piratpet )	/* KN  (c) 1982 Taito America Corporation */
	DRIVER( alpine )	/* RH  (c) 1982 Taito Corporation */
	DRIVER( alpinea )	/* RH  (c) 1982 Taito Corporation */
	DRIVER( timetunl )	/* UN  (c) 1982 Taito Corporation */
	DRIVER( wwestern )	/* WW  (c) 1982 Taito Corporation */
	DRIVER( wwester1 )	/* WW  (c) 1982 Taito Corporation */
	DRIVER( frontlin )	/* FL  (c) 1982 Taito Corporation */
	DRIVER( elevator )	/* EA  (c) 1983 Taito Corporation */
	DRIVER( elevatob )	/* bootleg */
	DRIVER( tinstar )	/* A10 (c) 1983 Taito Corporation */
	DRIVER( waterski )	/* A03 (c) 1983 Taito Corporation */
	DRIVER( bioatack )	/* AA8 (c) 1983 Taito Corporation + Fox Video Games license */
	DRIVER( hwrace )	/* AC4 (c) 1983 Taito Corporation */
	DRIVER( sfposeid )	/* A14 (c) 1984 Taito Corporation */
	DRIVER( kikstart )	/* A20 (c) 1984 Taito Corporation */

	/* other Taito games */
	DRIVER( fgoal )		/*     (c) 1979 Taito Corporation */
	DRIVER( fgoala )	/*     (c) 1979 Taito Corporation */
	DRIVER( crbaloon )	/* CL  (c) 1980 Taito Corporation */
	DRIVER( crbalon2 )	/* CL  (c) 1980 Taito Corporation */
	DRIVER( sbowling )	/* KB  (c) 1982 Taito Corporation */
	DRIVER( grchamp )	/* GM  (c) 1981 Taito Corporation */
	DRIVER( marinedt )	/* ??? (c) 1981 Taito Corporation */
	DRIVER( changela )	/* ??? (c) 1983 Taito Corporation */
	DRIVER( jollyjgr )	/* KD  (c) 1982 Taito Corporation */
	DRIVER( bking )		/* DM  (c) 1982 Taito Corporation */
	DRIVER( bking2 )	/* AD6 (c) 1983 Taito Corporation */
	DRIVER( bking3 )	/* A24 (c) 1984 Taito Corporation */
	DRIVER( chaknpop )	/* A04 (c) 1983 Taito Corporation */
	DRIVER( josvolly )	/* ??? (c) 1983 Taito Corporation */
	DRIVER( gsword )	/* ??? (c) 1984 Taito Corporation */
	DRIVER( gsword2 )	/* ??? (c) 1984 Taito Corporation */
	DRIVER( pitnrun )	/* ??? (c) 1984 Taito Corporation */
	DRIVER( pitnruna )	/* ??? (c) 1984 Taito Corporation */
	DRIVER( lkage )		/* A54 (c) 1984 Taito Corporation */
	DRIVER( lkageb )	/* bootleg */
	DRIVER( lkageb2 )	/* bootleg */
	DRIVER( lkageb3 )	/* bootleg */
	DRIVER( msisaac )	/* A34 (c) 1985 Taito Corporation */
	DRIVER( retofinv )	/* A37 (c) 1985 Taito Corporation */
	DRIVER( retofin1 )	/* bootleg */
	DRIVER( retofin2 )	/* bootleg */
	DRIVER( fightrol )	/* (c) 1983 Taito */
	DRIVER( rollace )	/* (c) 1983 Williams */
	DRIVER( rollace2 )	/* (c) 1983 Williams */
	DRIVER( vsgongf )	/* (c) 1984 Kaneko */
	DRIVER( ringfgt )	/* (c) 1984 Taito */
	DRIVER( ringfgt2 )	/* (c) 1984 Taito */
	DRIVER( fieldday )	/* A23 (c) 1984 Taito */
	DRIVER( undoukai )	/* A17 (c) 1984 Taito */
	DRIVER( 40love )	/* A30 (c) 1984 Taito */
	DRIVER( tsamurai )	/* A35 (c) 1985 Taito */
	DRIVER( tsamura2 )	/* A35 (c) 1985 Taito */
	DRIVER( nunchaku )	/* ??? (c) 1985 Taito */
	DRIVER( yamagchi )	/* A38 (c) 1985 Taito */
	DRIVER( m660 )		/* ??? (c) 1986 Taito America Corporation */
	DRIVER( m660j )		/* ??? (c) 1986 Taito Corporation (Japan) */
	DRIVER( m660b )		/* bootleg */
	DRIVER( alphaxz )	/* ??? (c) 1986 Ed/Wood Place */
	DRIVER( buggychl )	/* A22 (c) 1984 Taito Corporation */
	DRIVER( buggycht )	/* A22 (c) 1984 Taito Corporation + Tefri license */
	DRIVER( ssrj )		/* A40 (c) 1985 Taito Corporation */
	DRIVER( bigevglf )	/* A67 (c) 1986 Taito America Corporation (US) */
	DRIVER( flstory )	/* A45 (c) 1985 Taito Corporation */
	DRIVER( flstoryj )	/* A45 (c) 1985 Taito Corporation (Japan) */
	DRIVER( onna34ro )	/* A52 (c) 1985 Taito Corporation (Japan) */
	DRIVER( onna34ra )	/* A52 (c) 1985 Taito Corporation (Japan) */
	DRIVER( victnine )	/* A16 (c) 1985 Taito Corporation (Japan) */
	DRIVER( gladiatr )	/* QB  (c) 1986 Taito America Corporation (US) */
	DRIVER( ogonsiro )	/* QB  (c) 1986 Taito Corporation (Japan) */
	DRIVER( greatgur )	/* QB  (c) 1986 Taito Corporation (Japan?) */
	DRIVER( ppking )
	DRIVER( nycaptor )	/* A50 (c) 1985 Taito Corporation */
	DRIVER( cyclshtg )	/* A97 (c) 1986 Taito Corporation */
	DRIVER( bronx )		/* bootleg */
	DRIVER( colt )		/* bootleg */
	DRIVER( ksayakyu )	/* ??? (c) 1985 Taito Corporation */
	DRIVER( benberob )	/* A26 */
	DRIVER( halleys )	/* A62 (c) 1986 Taito America Corporation + Coin It (US) */
	DRIVER( halleysc )	/* A62 (c) 1986 Taito Corporation (Japan) */
	DRIVER( halleycj )	/* A62 (c) 1986 Taito Corporation (Japan) */
	DRIVER( lsasquad )	/* A64 (c) 1986 Taito Corporation / Taito America (dip switch) */
	DRIVER( storming )	/* A64 (c) 1986 Taito Corporation */
	DRIVER( tokio )		/* A71 1986 */
	DRIVER( tokiou )	/* A71 1986 */
	DRIVER( tokiob )	/* bootleg */
	DRIVER( bublbobl )	/* A78 (c) 1986 Taito Corporation */
	DRIVER( bublbob1 )	/* A78 (c) 1986 Taito Corporation */
	DRIVER( bublbobr )	/* A78 (c) 1986 Taito America Corporation + Romstar license */
	DRIVER( bubbobr1 )	/* A78 (c) 1986 Taito America Corporation + Romstar license */
	DRIVER( boblbobl )	/* bootleg */
	DRIVER( sboblbob )	/* bootleg */
	DRIVER( missb2 )	/* bootleg on enhanced hardware */
	DRIVER( kikikai )	/* A85 (c) 1986 Taito Corporation */
	DRIVER( knightb )	/* bootleg */
	DRIVER( kicknrun )	/* A87 (c) 1986 Taito Corporation */
	DRIVER( mexico86 )	/* bootleg (Micro Research) */
	DRIVER( darius )	/* A96 (c) 1986 Taito Corporation Japan (World) */
	DRIVER( dariusj )	/* A96 (c) 1986 Taito Corporation (Japan) */
	DRIVER( dariuso )	/* A96 (c) 1986 Taito Corporation (Japan) */
	DRIVER( dariuse )	/* A96 (c) 1986 Taito Corporation (Japan) */
	DRIVER( rastan )	/* B04 (c) 1987 Taito Corporation Japan (World) */
	DRIVER( rastanu )	/* B04 (c) 1987 Taito America Corporation (US) */
	DRIVER( rastanu2 )	/* B04 (c) 1987 Taito America Corporation (US) */
	DRIVER( rastsaga )	/* B04 (c) 1987 Taito Corporation (Japan)*/
	DRIVER( topspeed )	/* B14 (c) 1987 Taito Corporation Japan (World) */
	DRIVER( topspedu )	/* B14 (c) 1987 Taito America Corporation (US) */
	DRIVER( fullthrl )	/* B14 (c) 1987 Taito Corporation (Japan) */
	DRIVER( opwolf )	/* B20 (c) 1987 Taito Corporation Japan (World) */
	DRIVER( opwolfu )	/* B20 (c) 1987 Taito America Corporation (US) */
	DRIVER( opwolfb )	/* bootleg */
	DRIVER( othunder )	/* B67 (c) 1988 Taito Corporation Japan (World) */
	DRIVER( othundu )	/* B67 (c) 1988 Taito America Corporation (US) */
	DRIVER( othunduo )	/* B67 (c) 1988 Taito America Corporation (US) */
	DRIVER( othundrj )	/* B67 (c) 1988 Taito Corporation (Japan) */
	DRIVER( rainbow )	/* B22 (c) 1987 Taito Corporation */
	DRIVER( rainbowo )	/* B22 (c) 1987 Taito Corporation */
	DRIVER( rainbowe )	/* B39 (c) 1988 Taito Corporation */
	DRIVER( jumping )	/* bootleg */
	DRIVER( arkanoid )	/* A75 (c) 1986 Taito Corporation Japan (World) */
	DRIVER( arknoidu )	/* A75 (c) 1986 Taito America Corporation + Romstar license (US) */
	DRIVER( arknoiuo )	/* A75 (c) 1986 Taito America Corporation + Romstar license (US) */
	DRIVER( arknoidj )	/* A75 (c) 1986 Taito Corporation (Japan) */
	DRIVER( arkbl2 )	/* bootleg */
	DRIVER( arkbl3 )	/* bootleg */
	DRIVER( paddle2 )	/* bootleg */
	DRIVER( arkatayt )	/* bootleg */
	DRIVER( arkblock )	/* bootleg */
	DRIVER( arkbloc2 )	/* bootleg */
	DRIVER( arkangc )	/* bootleg */
	DRIVER( arkatour )	/* ??? (c) 1987 Taito America Corporation + Romstar license (US) */
	DRIVER( tetrsark )	/* ??? (c) D.R.Korea */
	DRIVER( sqix )		/* B03 1987 */
	DRIVER( sqixa )		/* B03 1987 */
	DRIVER( sqixbl )	/* bootleg? but (c) 1987 */
	DRIVER( perestro )	/* (c) 1994 Promat */
	DRIVER( perestrf )	/* (c) 1993 Promat / Fuuki */
	DRIVER( pbillian )	/* (c) 1986 Taito */
	DRIVER( hotsmash )	/* B18 (c) 1987 Taito */
	DRIVER( exzisus )	/* B23 (c) 1987 Taito Corporation (Japan) */
	DRIVER( minivadr )	/* D26 cabinet test board */
	DRIVER( volfied )	/* C04 (c) 1989 Taito Corporation Japan (World) */
	DRIVER( volfiedu )	/* C04 (c) 1989 Taito America Corporation (US) */
	DRIVER( volfiedj )	/* C04 (c) 1989 Taito Corporation (Japan) */
	DRIVER( bonzeadv )	/* B41 (c) 1988 Taito Corporation Japan (World) */
	DRIVER( bonzeado )	/* B41 (c) 1988 Taito Corporation Japan (World) */
	DRIVER( bonzeadu )	/* B41 (c) 1988 Taito America Corporation (US) */
	DRIVER( jigkmgri )	/* B41 (c) 1988 Taito Corporation (Japan)*/
	DRIVER( asuka )		/* B68 (c) 1988 Taito Corporation (Japan) */
	DRIVER( mofflott )	/* C17 (c) 1989 Taito Corporation (Japan) */
	DRIVER( cadash )	/* C21 (c) 1989 Taito Corporation Japan */
	DRIVER( cadashj )	/* C21 (c) 1989 Taito Corporation */
	DRIVER( cadashu )	/* C21 (c) 1989 Taito America Corporation */
	DRIVER( cadashi )	/* C21 (c) 1989 Taito Corporation Japan */
	DRIVER( cadashf )	/* C21 (c) 1989 Taito Corporation Japan */
	DRIVER( galmedes )	/* (c) 1992 Visco (Japan) */
	DRIVER( earthjkr )	/* (c) 1993 Visco (Japan) */
	DRIVER( eto )		/* (c) 1994 Visco (Japan) */
	DRIVER( wgp )		/* C32 (c) 1989 Taito America Corporation (US) */
	DRIVER( wgpj )		/* C32 (c) 1989 Taito Corporation (Japan) */
	DRIVER( wgpjoy )	/* C32 (c) 1989 Taito Corporation (Japan) */
	DRIVER( wgpjoya )	/* C32 (c) 1989 Taito Corporation (Japan) */
	DRIVER( wgp2 )		/* C73 (c) 1990 Taito Corporation (Japan) */
	DRIVER( slapshot )	/* D71 (c) 1994 Taito Corporation (Japan) */
	DRIVER( opwolf3 )	/* D74 (c) 1994 Taito */
	DRIVER( opwolf3u )	/* D74 (c) 1994 Taito */
	DRIVER( ashnojoe )	/* ??? (c) 1990 Wave / Taito */

	/* Taito multi-screen games */
	DRIVER( ninjaw )	/* B31 (c) 1987 Taito Corporation Japan (World) */
	DRIVER( ninjawj )	/* B31 (c) 1987 Taito Corporation (Japan) */
	DRIVER( darius2 )	/* C07 (c) 1989 Taito Corporation (Japan) */
	DRIVER( darius2d )	/* C07 (c) 1989 Taito Corporation (Japan) */
	DRIVER( drius2do )	/* C07 (c) 1989 Taito Corporation (Japan) */
	DRIVER( warriorb )	/* D24 (c) 1991 Taito Corporation (Japan) */

	/* Taito "X"-system games */
	DRIVER( superman )	/* B61 (c) 1988 Taito Corporation */
	DRIVER( suprmanj )	/* B61 (c) 1988 Taito Corporation */
	DRIVER( twinhawk )	/* B87 (c) 1989 Taito Corporation Japan (World) */
	DRIVER( twinhwku )	/* B87 (c) 1989 Taito America Corporation (US) */
	DRIVER( daisenpu )	/* B87 (c) 1989 Taito Corporation (Japan) */
	DRIVER( gigandes )	/* (c) 1989 East Technology */
	DRIVER( gigandsj )	/* (c) 1989 East Technology */
	DRIVER( kyustrkr )	/* (c) 1989 East Technology */
	DRIVER( ballbros )	/* no copyright notice */

	/* Taito "tnzs" (Seta) hardware */
	DRIVER( plumppop )	/* A98 (c) 1987 Taito Corporation (Japan) */
	DRIVER( extrmatn )	/* B06 (c) 1987 World Games */
	DRIVER( arknoid2 )	/* B08 (c) 1987 Taito Corporation Japan (World) */
	DRIVER( arknid2u )	/* B08 (c) 1987 Taito America Corporation + Romstar license (US) */
	DRIVER( arknid2j )	/* B08 (c) 1987 Taito Corporation (Japan) */
	DRIVER( drtoppel )	/* B19 (c) 1987 Taito Corporation Japan (World) */
	DRIVER( drtopplu )	/* B19 (c) 1987 Taito Corporation (US) */
	DRIVER( drtopplj )	/* B19 (c) 1987 Taito Corporation (Japan) */
	DRIVER( kageki )	/* B35 (c) 1988 Taito America Corporation + Romstar license (US) */
	DRIVER( kagekij )	/* B35 (c) 1988 Taito Corporation (Japan) */
	DRIVER( kagekia )	/* B35 (c) 1992 (hack?) */
	DRIVER( chukatai )	/* B44 (c) 1988 Taito Corporation Japan (World) */
	DRIVER( chukatau )	/* B44 (c) 1988 Taito Corporation (US) */
	DRIVER( chukataj )	/* B44 (c) 1988 Taito Corporation (Japan) */
	DRIVER( kabukiz )	/* B50 (c) 1988 Taito Corporation Japan (World) */
	DRIVER( kabukizj )	/* B50 (c) 1988 Taito Corporation (Japan) */
	DRIVER( tnzs )		/* B53 (c) 1988 Taito Corporation Japan (World) (new logo) */
	DRIVER( tnzsj )		/* B53 (c) 1988 Taito Corporation (Japan) (new logo) */
	DRIVER( tnzso )		/* B53?(c) 1988 Taito Corporation Japan (World) (old logo) */
	DRIVER( insectx )	/* B97 (c) 1989 Taito Corporation Japan (World) */
	DRIVER( champbwl )	/* (c)1989 Romstar.  not Taito, but the same Seta video chips */

	/* Taito L-System games */
	DRIVER( raimais )	/* B36 (c) 1988 Taito Corporation (Japan) */
	DRIVER( raimaisj )	/* B36 (c) 1988 Taito Corporation */
	DRIVER( kurikint )	/* B42 (c) 1988 Taito Corporation Japan (World) */
	DRIVER( kurikinu )	/* B42 (c) 1988 Taito America Corporation (US) */
	DRIVER( kurikinj )	/* B42 (c) 1988 Taito Corporation (Japan) */
	DRIVER( kurikina )	/* B42 (c) 1988 Taito Corporation Japan (World) */
	DRIVER( evilston )	/* C67 (c) 1990 Spacy Industrial, Ltd */
	DRIVER( fhawk )		/* B70 (c) 1988 Taito Corporation Japan (World) */
	DRIVER( fhawkj )	/* B70 (c) 1988 Taito Corporation (Japan) */
	DRIVER( plotting )	/* B96 (c) 1989 Taito Corporation Japan (World) */
	DRIVER( plottinu )	/* B96 (c) 1989 Taito Corporation Japan (US) */
	DRIVER( plottina )	/* B96 (c) 1989 Taito Corporation Japan (World) */
	DRIVER( champwr )	/* C01 (c) 1989 Taito Corporation Japan (World) */
	DRIVER( champwru )	/* C01 (c) 1989 Taito America Corporation (US) */
	DRIVER( champwrj )	/* C01 (c) 1989 Taito Corporation (Japan) */
	DRIVER( puzznic )	/* C20 (c) 1989 Taito Corporation (Japan) */
	DRIVER( puzznicj )	/* C20 (c) 1989 Taito Corporation (Japan) */
	DRIVER( horshoes )	/* C47 (c) 1990 Taito America Corporation (US) */
	DRIVER( palamed )	/* C63 (c) 1990 Taito Corporation (Japan) */
	DRIVER( cachat )	/* ??? (c) 1993 Taito Corporation (Japan) */
	DRIVER( tubeit )	/* ??? no copyright message */
	DRIVER( cubybop )	/* ??? no copyright message */
	DRIVER( plgirls )	/* (c) 1992 Hot-B. */
	DRIVER( plgirls2 )	/* (c) 1993 Hot-B. */

	/* Taito H-System games */
	DRIVER( syvalion )	/* B51 (c) 1988 Taito Corporation (Japan) */
	DRIVER( recordbr )	/* B56 (c) 1988 Taito Corporation Japan (World) */
	DRIVER( dleague )	/* C02 (c) 1990 Taito Corporation (Japan) */

	/* Taito B-System games */
	DRIVER( masterw )	/* B72 (c) 1989 Taito Corporation Japan (World) */
	DRIVER( masterwu )	/* B72 (c) 1989 America Corporation (US) */
	DRIVER( nastar )	/* B81 (c) 1988 Taito Corporation Japan (World) */
	DRIVER( nastarw )	/* B81 (c) 1988 Taito America Corporation (US) */
	DRIVER( rastsag2 )	/* B81 (c) 1988 Taito Corporation (Japan) */
	DRIVER( rambo3 )	/* B93 (c) 1989 Taito Europe Corporation (Europe) */
	DRIVER( rambo3ae )	/* B93 (c) 1989 Taito Europe Corporation (Europe) */
	DRIVER( rambo3a )	/* B93 (c) 1989 Taito America Corporation (US) */
	DRIVER( crimec )	/* B99 (c) 1989 Taito Corporation Japan (World) */
	DRIVER( crimecu )	/* B99 (c) 1989 Taito America Corporation (US) */
	DRIVER( crimecj )	/* B99 (c) 1989 Taito Corporation (Japan) */
	DRIVER( tetrist )	/* C12 (c) 1989 Sega Enterprises,Ltd. (Japan) */
	DRIVER( tetrista )	/* C35 (c) 1989 Sega Enterprises,Ltd. (Japan) */
	DRIVER( viofight )	/* C16 (c) 1989 Taito Corporation Japan (World) */
	DRIVER( viofighu )	/* C16 (c) 1989 Taito America Corporation (US) */
	DRIVER( ashura )	/* C43 (c) 1990 Taito Corporation (Japan) */
	DRIVER( ashurau )	/* C43 (c) 1990 Taito America Corporation (US) */
	DRIVER( hitice )	/* C59 (c) 1990 Williams (US) */
	DRIVER( sbm )		/* C69 (c) 1990 Taito Corporation (Japan) */
	DRIVER( selfeena )	/* ??? (c) 1991 East Technology */
	DRIVER( silentd )	/* ??? (c) 1992 Taito Corporation Japan (World) */
	DRIVER( silentdj )	/* ??? (c) 1992 Taito Corporation (Japan) */
	DRIVER( ryujin )	/* ??? (c) 1993 Taito Corporation (Japan) */
	DRIVER( qzshowby )	/* D72 (c) 1993 Taito Corporation (Japan) */
	DRIVER( pbobble )	/* ??? (c) 1994 Taito Corporation (Japan) */
	DRIVER( spacedx )	/* D89 (c) 1994 Taito Corporation (US) */
	DRIVER( spacedxj )	/* D89 (c) 1994 Taito Corporation (Japan) */
	DRIVER( spacedxo )	/* D89 (c) 1994 Taito Corporation (Japan) */

	/* Taito Z-System games */
	DRIVER( contcirc )	/* B33 (c) 1987 Taito Corporation Japan (World) */
	DRIVER( contcrcu )	/* B33 (c) 1987 Taito America Corporation (US) */
	DRIVER( contcrua )	/* B33 (c) 1987 Taito America Corporation (US) */
	DRIVER( chasehq )	/* B52 (c) 1988 Taito Corporation Japan (World) */
	DRIVER( chasehqj )	/* B52 (c) 1988 Taito Corporation (Japan) */
	DRIVER( enforce )	/* B58 (c) 1988 Taito Corporation (Japan) */
	DRIVER( nightstr )	/* B91 (c) 1989 Taito America Corporation (US) */
	DRIVER( sci )		/* C09 (c) 1989 Taito Corporation Japan (World) */
	DRIVER( scia )		/* C09 (c) 1989 Taito Corporation Japan (World) */
	DRIVER( sciu )		/* C09 (c) 1989 Taito America Corporation (US) */
	DRIVER( bshark )	/* C34 (c) 1989 Taito America Corporation (US) */
	DRIVER( bsharkj )	/* C34 (c) 1989 Taito Corporation (Japan) */
	DRIVER( aquajack )	/* B77 (c) 1990 Taito Corporation Japan (World) */
	DRIVER( aquajckj )	/* B77 (c) 1990 Taito Corporation (Japan) */
	DRIVER( spacegun )	/* C57 (c) 1990 Taito Corporation Japan (World) */
	DRIVER( dblaxle )	/* C78 (c) 1991 Taito America Corporation (US) */
	DRIVER( pwheelsj )	/* C78 (c) 1991 Taito Corporation (Japan) */
	DRIVER( racingb )	/* C84 (c) 1991 Taito Coropration .. */

	/* Taito Air System games */
	DRIVER( topland )	/* B62 (c) 1988 Taito Coporation Japan (World) */
	DRIVER( ainferno )	/* C45 (c) 1990 Taito America Corporation (US) */

	/* enhanced Z-System hardware games */
	DRIVER( gunbustr )	/* D27 (c) 1992 Taito Corporation (Japan) */
	DRIVER( superchs )	/* D46 (c) 1992 Taito America Corporation (US) */
	DRIVER( groundfx )	/* D51 (c) 1992 Taito Coporation */
	DRIVER( undrfire )	/* D67 (c) 1993 Taito Coporation Japan (World) */
	DRIVER( undrfiru )	/* D67 (c) 1993  Taito America Corporation (US) */
	DRIVER( undrfirj )	/* D67 (c) 1993 Taito Coporation (Japan) */

	/* Taito F2 games */
	DRIVER( finalb )	/* B82 (c) 1988 Taito Corporation Japan (World) */
	DRIVER( finalbj )	/* B82 (c) 1988 Taito Corporation (Japan) */
	DRIVER( finalbu )	/* B82 (c) 1988 Taito America Corporation (US) */
	DRIVER( dondokod )	/* B95 (c) 1989 Taito Corporation Japan (World) */
	DRIVER( dondokdu )	/* B95 (c) 1989 Taito America Corporation (US) */
	DRIVER( dondokdj )	/* B95 (c) 1989 Taito Corporation (Japan) */
	DRIVER( megab )		/* C11 (c) 1989 Taito Corporation Japan (World) */
	DRIVER( megabj )	/* C11 (c) 1989 Taito Corporation (Japan) */
	DRIVER( thundfox )	/* C28 (c) 1990 Taito Corporation Japan (World) */
	DRIVER( thndfoxu )	/* C28 (c) 1990 Taito America Corporation (US) */
	DRIVER( thndfoxj )	/* C28 (c) 1990 Taito Corporation (Japan) */
	DRIVER( cameltry )	/* C38 (c) 1989 Taito America Corporation (US) */
	DRIVER( camltrua )	/* C38 (c) 1989 Taito America Corporation (US) */
	DRIVER( cameltrj )	/* C38 (c) 1989 Taito Corporation (Japan) */
	DRIVER( qtorimon )	/* C41 (c) 1990 Taito Corporation (Japan) */
	DRIVER( liquidk )	/* C49 (c) 1990 Taito Corporation Japan (World) */
	DRIVER( liquidku )	/* C49 (c) 1990 Taito America Corporation (US) */
	DRIVER( mizubaku )	/* C49 (c) 1990 Taito Corporation (Japan) */
	DRIVER( quizhq )	/* C53 (c) 1990 Taito Corporation (Japan) */
	DRIVER( ssi )		/* C64 (c) 1990 Taito Corporation Japan (World) */
	DRIVER( majest12 )	/* C64 (c) 1990 Taito Corporation (Japan) */
	DRIVER( gunfront )	/* C71 (c) 1990 Taito Corporation Japan (World) */
	DRIVER( gunfronj )	/* C71 (c) 1990 Taito Corporation (Japan) */
	DRIVER( growl )		/* C74 (c) 1990 Taito Corporation Japan (World) */
	DRIVER( growlu )	/* C74 (c) 1990 Taito America Corporation (US) */
	DRIVER( runark )	/* C74 (c) 1990 Taito Corporation (Japan) */
	DRIVER( mjnquest )	/* C77 (c) 1990 Taito Corporation (Japan) */
	DRIVER( mjnquesb )	/* C77 (c) 1990 Taito Corporation (Japan) */
	DRIVER( footchmp )	/* C80 (c) 1990 Taito Corporation Japan (World) */
	DRIVER( hthero )	/* C80 (c) 1990 Taito Corporation (Japan) */
	DRIVER( euroch92 )	/*     (c) 1992 Taito Corporation Japan (World) */
	DRIVER( koshien )	/* C81 (c) 1990 Taito Corporation (Japan) */
	DRIVER( yuyugogo )	/* C83 (c) 1990 Taito Corporation (Japan) */
	DRIVER( ninjak )	/* C85 (c) 1990 Taito Corporation Japan (World) */
	DRIVER( ninjakj )	/* C85 (c) 1990 Taito Corporation (Japan) */
	DRIVER( ninjaku )	/* C85 (c) 1990 Taito Corporation (Japan) */
	DRIVER( solfigtr )	/* C91 (c) 1991 Taito Corporation Japan (World) */
	DRIVER( qzquest )	/* C92 (c) 1991 Taito Corporation (Japan) */
	DRIVER( pulirula )	/* C98 (c) 1991 Taito Corporation Japan (World) */
	DRIVER( pulirulj )	/* C98 (c) 1991 Taito Corporation (Japan) */
	DRIVER( metalb )	/* D16? (c) 1991 Taito Corporation Japan (World) */
	DRIVER( metalbj )	/* D12 (c) 1991 Taito Corporation (Japan) */
	DRIVER( qzchikyu )	/* D19 (c) 1991 Taito Corporation (Japan) */
	DRIVER( yesnoj )	/* D20 (c) 1992 Taito Corporation (Japan) */
	DRIVER( deadconx )	/* D28 (c) 1992 Taito Corporation Japan (World) */
	DRIVER( deadconj )	/* D28 (c) 1992 Taito Corporation (Japan) */
	DRIVER( dinorex )	/* D39 (c) 1992 Taito Corporation Japan (World) */
	DRIVER( dinorexj )	/* D39 (c) 1992 Taito Corporation (Japan) */
	DRIVER( dinorexu )	/* D39 (c) 1992 Taito America Corporation (US) */
	DRIVER( qjinsei )	/* D48 (c) 1992 Taito Corporation (Japan) */
	DRIVER( qcrayon )	/* D55 (c) 1993 Taito Corporation (Japan) */
	DRIVER( qcrayon2 )	/* D63 (c) 1993 Taito Corporation (Japan) */
	DRIVER( driftout )	/* (c) 1991 Visco */
	DRIVER( driveout )	/* bootleg */

	/* Taito F3 games */
	DRIVER( ringrage )	/* D21 (c) 1992 Taito Corporation Japan (World) */
	DRIVER( ringragj )	/* D21 (c) 1992 Taito Corporation (Japan) */
	DRIVER( ringragu )	/* D21 (c) 1992 Taito America Corporation (US) */
	DRIVER( arabianm )	/* D29 (c) 1992 Taito Corporation Japan (World) */
	DRIVER( arabiamj )	/* D29 (c) 1992 Taito Corporation (Japan) */
	DRIVER( arabiamu )	/* D29 (c) 1992 Taito America Corporation (US) */
	DRIVER( ridingf )	/* D34 (c) 1992 Taito Corporation Japan (World) */
	DRIVER( ridefgtj )	/* D34 (c) 1992 Taito Corporation (Japan) */
	DRIVER( ridefgtu )	/* D34 (c) 1992 Taito America Corporation (US) */
	DRIVER( gseeker )	/* D40 (c) 1992 Taito Corporation Japan (World) */
	DRIVER( gseekerj )	/* D40 (c) 1992 Taito Corporation (Japan) */
	DRIVER( gseekeru )	/* D40 (c) 1992 Taito America Corporation (US) */
	DRIVER( hthero93 )	/* D49 (c) 1992 Taito Corporation (Japan) */
	DRIVER( cupfinal )	/* D49 (c) 1993 Taito Corporation Japan (World) */
	DRIVER( trstar )	/* D53 (c) 1993 Taito Corporation Japan (World) */
	DRIVER( trstarj )	/* D53 (c) 1993 Taito Corporation (Japan) */
	DRIVER( prmtmfgt )	/* D53 (c) 1993 Taito Corporation (US) */
	DRIVER( trstaro )	/* D53 (c) 1993 Taito Corporation (World) */
	DRIVER( trstaroj )	/* D53 (c) 1993 Taito Corporation (Japan) */
	DRIVER( prmtmfgo )	/* D53 (c) 1993 Taito Corporation (US) */
	DRIVER( gunlock )	/* D66 (c) 1993 Taito Corporation Japan (World) */
	DRIVER( rayforcj )	/* D66 (c) 1993 Taito Corporation (Japan) */
	DRIVER( rayforce )	/* D66 (c) 1993 Taito America Corporation (US) */
	DRIVER( scfinals )	/* D68 (c) 1993 Taito Corporation Japan (World) */
	DRIVER( intcup94 )	/* D78 (c) 1994 Taito */
	DRIVER( hthero94 )	/* D78 (c) 1994 Taito */
	DRIVER( dungeonm )	/* D69 (c) 1993 Taito Corporation Japan (World) */
	DRIVER( lightbr )	/* D69 (c) 1993 Taito Corporation (Japan) */
	DRIVER( dungenmu )	/* D69 (c) 1993 Taito America Corporation (US) */
	DRIVER( kaiserkn )	/* D84 (c) 1994 Taito Corporation Japan (World) */
	DRIVER( kaiserkj )	/* D84 (c) 1994 Taito Corporation (Japan) */
	DRIVER( gblchmp )	/* D84 (c) 1994 Taito America Corporation (US) */
	DRIVER( dankuga )	/* D84? (c) 1994 Taito Corporation (Japan) */
	DRIVER( dariusg )	/* D87 (c) 1994 Taito Corporation Japan (World) */
	DRIVER( dariusgj )	/* D87 (c) 1994 Taito Corporation (Japan) */
	DRIVER( dariusgu )	/* D87 (c) 1994 Taito America Corporation (US) */
	DRIVER( dariusgx )	/* D87 (c) 1994 Taito Corporation */
	DRIVER( bublbob2 )	/* D90 (c) 1994 Taito Corporation Japan (World) */
	DRIVER( bubsympe )	/* D90 (c) 1994 Taito Corporation Japan (Europe) */
	DRIVER( bubsympu )	/* D90 (c) 1994 Taito America Corporation (US) */
	DRIVER( bubsymph )	/* D90 (c) 1994 Taito Corporation (Japan) */
	DRIVER( spcinvdj )	/* D93 (c) 1994 Taito Corporation (Japan) */
	DRIVER( pwrgoal )	/* D94 (c) 1995 Taito Corporation Japan (World) */
	DRIVER( hthero95 )	/* D94 (c) 1995 Taito Corporation (Japan) */
	DRIVER( hthro95u )	/* D94 (c) 1995 Taito America Corporation (US) */
	DRIVER( qtheater )	/* D95 (c) 1994 Taito Corporation (Japan) */
	DRIVER( elvactr )	/* E02 (c) 1994 Taito Corporation Japan (World) */
	DRIVER( elvactrj )	/* E02 (c) 1994 Taito Corporation (Japan) */
	DRIVER( elvact2u )	/* E02 (c) 1994 Taito America Corporation (US) */
	DRIVER( spcinv95 )	/* E06 (c) 1995 Taito Corporation Japan (World) */
	DRIVER( spcnv95u )	/* E06 (c) 1995 Taito America Corporation (US) */
	DRIVER( akkanvdr )	/* E06 (c) 1995 Taito Corporation (Japan) */
	DRIVER( twinqix )	/* ??? (c) 1995 Taito America Corporation (US) */
	DRIVER( quizhuhu )	/* E08 (c) 1995 Taito Corporation (Japan) */
	DRIVER( pbobble2 )	/* E10 (c) 1995 Taito Corporation Japan (World) */
	DRIVER( pbobbl2j )	/* E10 (c) 1995 Taito Corporation (Japan) */
	DRIVER( pbobbl2u )	/* E10 (c) 1995 Taito America Corporation (US) */
	DRIVER( pbobbl2x )	/* E10 (c) 1995 Taito Corporation (Japan) */
	DRIVER( gekirido )	/* E11 (c) 1995 Taito Corporation (Japan) */
	DRIVER( tcobra2 )	/* E15 (c) 1995 Taito Corporation (Japan) */
	DRIVER( tcobra2u )	/* E15 (c) 1995 Taito Corporation (Japan) */
	DRIVER( ktiger2 )	/* E15 (c) 1995 Taito Corporation (Japan) */
	DRIVER( bubblem )	/* E21 (c) 1995 Taito Corporation Japan (World) */
	DRIVER( bubblemj )	/* E21 (c) 1995 Taito Corporation (Japan) */
	DRIVER( cleopatr )	/* E28 (c) 1996 Taito Corporation (Japan) */
	DRIVER( pbobble3 )	/* E29 (c) 1996 Taito Corporation (World) */
	DRIVER( pbobbl3u )	/* E29 (c) 1996 Taito Corporation (US) */
	DRIVER( pbobbl3j )	/* E29 (c) 1996 Taito Corporation (Japan) */
	DRIVER( arkretrn )	/* E36 (c) 1997 Taito Corporation (Japan) */
	DRIVER( kirameki )	/* E44 (c) 1997 Taito Corporation (Japan) */
	DRIVER( puchicar )	/* E46 (c) 1997 Taito Corporation (World) */
	DRIVER( puchicrj )	/* E46 (c) 1997 Taito Corporation (Japan) */
	DRIVER( pbobble4 )	/* E49 (c) 1997 Taito Corporation (World) */
	DRIVER( pbobbl4j )	/* E49 (c) 1997 Taito Corporation (Japan) */
	DRIVER( pbobbl4u )	/* E49 (c) 1997 Taito Corporation (US) */
	DRIVER( popnpop )	/* E51 (c) 1997 Taito Corporation (World) */
	DRIVER( popnpopj )	/* E51 (c) 1997 Taito Corporation (Japan) */
	DRIVER( popnpopu )	/* E51 (c) 1997 Taito Corporation (US) */
	DRIVER( landmakr )	/* E61 (c) 1998 Taito Corporation (Japan) */
	DRIVER( landmkrp )	/* E61 (c) 1998 Taito Corporation (World, prototype) */
	DRIVER( recalh )	/* prototype */
	DRIVER( commandw )	/* prototype */

	/* Taito JC System */
	DRIVER( landgear )	/* E17 (c) 199? Taito Corporation */
	DRIVER( dendeg )	/* E35 (c) 199? Taito Corporation */
	DRIVER( dendegx )	/* E35 (c) 199? Taito Corporation */
	DRIVER( dendeg2 )	/* E52 (c) 199? Taito Corporation */
	DRIVER( dendeg2x )	/* E52 (c) 199? Taito Corporation */
	DRIVER( sidebs )	/* E23 (c) 199? Taito Corporation */
	DRIVER( sidebs2 )	/* E38 (c) 199? Taito Corporation */

	/* Taito "Wolf" System */
	DRIVER( pf2012 )	/* E59 (c) 1997 Taito */

	/* Toaplan games */
	DRIVER( perfrman )	/* (c) 1985 Data East Corporation (Japan) */
	DRIVER( perfrmau )	/* (c) 1985 Data East USA (US) */
	DRIVER( tigerh )	/* A47 (c) 1985 Taito America  Corporation   GX-551 [not a Konami board!] */
	DRIVER( tigerh2 )	/* A47 (c) 1985 Taito Corporation   GX-551 [not a Konami board!] */
	DRIVER( tigerhj )	/* A47 (c) 1985 Taito Corporation   GX-551 [not a Konami board!] */
	DRIVER( tigerhb1 )	/* bootleg but (c) 1985 Taito Corporation */
	DRIVER( tigerhb2 )	/* bootleg but (c) 1985 Taito Corporation */
	DRIVER( slapfigh )	/* TP-??? */
	DRIVER( slapfiga )	/* TP-??? */
	DRIVER( slapbtjp )	/* bootleg but (c) 1986 Taito Corporation */
	DRIVER( slapbtuk )	/* bootleg but (c) 1986 Taito Corporation */
	DRIVER( slapfgtr )	/* bootleg */
	DRIVER( alcon )		/* TP-??? */
	DRIVER( getstar )	/* A68 (c) 1986 Taito Corporation */
	DRIVER( getstarj )	/* A68 (c) 1986 Taito Corporation */
	DRIVER( getstarb )	/* GX-006 bootleg but (c) 1986 Taito Corporation */
	DRIVER( gtstarba )	/* GX-006 bootleg but (c) 1986 Taito Corporation */
	DRIVER( mjsister )	/* (c) 1986 Toaplan */

	DRIVER( fshark )	/* TP-007 (c) 1987 Taito Corporation (World) */
	DRIVER( skyshark )	/* TP-007 (c) 1987 Taito America Corporation + Romstar license (US) */
	DRIVER( hishouza )	/* TP-007 (c) 1987 Taito Corporation (Japan) */
	DRIVER( fsharkbt )	/* bootleg */
	DRIVER( wardner )	/* TP-009 (c) 1987 Taito Corporation Japan (World) */
	DRIVER( pyros )		/* TP-009 (c) 1987 Taito America Corporation (US) */
	DRIVER( wardnerj )	/* TP-009 (c) 1987 Taito Corporation (Japan) */
	DRIVER( twincobr )	/* TP-011 (c) 1987 Taito Corporation (World) */
	DRIVER( twincobu )	/* TP-011 (c) 1987 Taito America Corporation + Romstar license (US) */
	DRIVER( ktiger )	/* TP-011 (c) 1987 Taito Corporation (Japan) */
	DRIVER( gulfwar2 )	/* (c) 1991 Comad */

	DRIVER( rallybik )	/* TP-012 (c) 1988 Taito */
	DRIVER( truxton )	/* TP-013B (c) 1988 Taito */
	DRIVER( hellfire )	/* TP-??? (c) 1989 Toaplan + Taito license */
	DRIVER( hellfir1 )	/* TP-??? (c) 1989 Toaplan + Taito license */

	/* AdvanceMAME new games from ThunderMAME32Plus-v0.106X/ThunderMAME32Plus-2015-09-02-src.7z */
	DRIVER( hellfir2 )	/* TP-??? (c) 1989 Toaplan + Taito license */
	DRIVER( hellfir3 )	/* TP-??? (c) 1989 Toaplan + Taito license */

	DRIVER( zerowing )	/* TP-015 (c) 1989 Toaplan */

	/* AdvanceMAME new games from ThunderMAME32Plus-v0.106X/ThunderMAME32Plus-2015-09-02-src.7z */
	DRIVER( zerowng2 )	/* TP-015 (c) 1989 Toaplan */

	DRIVER( demonwld )	/* TP-016 (c) 1990 Toaplan (+ Taito license when set to Japan) */
	DRIVER( demonwl1 )	/* TP-016 (c) 1989 Toaplan + Taito license */
	DRIVER( demonwl2 )	/* TP-016 (c) 1989 Toaplan */

	/* AdvanceMAME new games from ThunderMAME32Plus-v0.106X/ThunderMAME32Plus-2015-09-02-src.7z */
	DRIVER( demonwl3 )	/* TP-016 (c) 1989 Toaplan */
	DRIVER( demonwl4 )	/* TP-016 (c) 1989 Toaplan */

	DRIVER( fireshrk )	/* TP-017 (c) 1990 Toaplan */
	DRIVER( samesame )	/* TP-017 (c) 1989 Toaplan */
	DRIVER( samesam2 )	/* TP-017 (c) 1989 Toaplan */
	DRIVER( outzone )	/* TP-018 (c) 1990 Toaplan */
	DRIVER( outzonea )	/* TP-018 (c) 1990 Toaplan */
	DRIVER( outzoneb )	/* TP-018 (c) 1990 Toaplan */
	DRIVER( outzonec )	/* TP-018 (c) 1990 Toaplan */

	/* AdvanceMAME new games from ThunderMAME32Plus-v0.106X/ThunderMAME32Plus-2015-09-02-src.7z */
	DRIVER( outzoned )	/* TP-018 (c) 1990 Toaplan */

	DRIVER( vimana )	/* TP-019 (c) 1991 Toaplan (+ Tecmo license when set to Japan) */
	DRIVER( vimana1 )	/* TP-019 (c) 1991 Toaplan (+ Tecmo license when set to Japan)  */
	DRIVER( vimanan )	/* TP-019 (c) 1991 Toaplan (+ Nova Apparate GMBH & Co license) */
	DRIVER( snowbros )	/* MIN16-02 (c) 1990 Toaplan + Romstar license */
	DRIVER( snowbroa )	/* MIN16-02 (c) 1990 Toaplan + Romstar license */
	DRIVER( snowbrob )	/* MIN16-02 (c) 1990 Toaplan + Romstar license */
	DRIVER( snowbroc )	/* MIN16-02 (c) 1990 Toaplan + Romstar license */
	DRIVER( snowbroj )	/* MIN16-02 (c) 1990 Toaplan */
	DRIVER( wintbob )	/* bootleg */
	DRIVER( honeydol )	/* (c) 1995 Barko Corp */
		/* SemiCom games on snowbros like hardware */
	DRIVER( hyperpac )	/* (c) 1995 SemiCom */
	DRIVER( hyperpcb )	/* bootleg */
	DRIVER( twinkle )	/* (c) 1997 SemiCom */
	DRIVER( toppyrap )	/* (c) 1996 SemiCom */
	DRIVER( cookbib2 )	/* (c) 1996 SemiCom */
	DRIVER( cookbib3 )	/* (c) 1997 SemiCom */
	DRIVER( 3in1semi )	/* (c) 1998 SemiCom */
	DRIVER( moremore )	/* (c) 1999 SemiCom + Exit */
	DRIVER( moremorp )	/* (c) 1999 SemiCom + Exit */
	DRIVER( 4in1boot )	/* (c) 2002 but bootleg of 1999 release? */
	DRIVER( snowbro3 )	/* (c) 2002 but hack / bootleg of snowbros? */
	DRIVER( finalttr )	/* (c) 1993 Jeil Computer System */

	/* More Toaplan Games */
	DRIVER( tekipaki )	/* TP-020 (c) 1991 Toaplan */
	DRIVER( ghox )		/* TP-021 (c) 1991 Toaplan */
	DRIVER( dogyuun )	/* TP-022 (c) 1992 Toaplan */
	DRIVER( kbash )		/* TP-023 (c) 1993 Toaplan */
	DRIVER( kbash2 )	/* bootleg */
	DRIVER( truxton2 )	/* TP-024 (c) 1992 Toaplan */
	DRIVER( pipibibs )	/* TP-025 */
	DRIVER( whoopee )	/* TP-025 */
	DRIVER( pipibibi )	/* (c) 1991 Ryouta Kikaku (bootleg?) */
	DRIVER( fixeight )	/* TP-026 (c) 1992 + Taito license */
	DRIVER( fixeighb )	/* bootleg */
	DRIVER( vfive )		/* TP-027 (c) 1993 Toaplan (Japan) */
	DRIVER( grindstm )	/* TP-027 (c) 1993 Toaplan + Unite Trading license (Korea) */
	DRIVER( grindsta )	/* TP-027 (c) 1993 Toaplan + Unite Trading license (Korea) */
	DRIVER( batsugun )	/* TP-030 (c) 1993 Toaplan */
	DRIVER( batsugna )	/* TP-030 (c) 1993 Toaplan */
	DRIVER( batugnsp )	/* TP-??? (c) 1993 Toaplan */
	DRIVER( snowbro2 )	/* TP-??? (c) 1994 Hanafram */
		/* see http://www.vsa-ag.ch/r8zing/ for a list of Raizing/8ing games */
	DRIVER( sstriker )	/* (c) 1993 Raizing */
	DRIVER( sstrikra )	/* (c) 1993 Raizing */
	DRIVER( mahoudai )	/* (c) 1993 Raizing + Able license */
	DRIVER( shippumd )	/* (c) 1994 Raizing/8ing */
	DRIVER( kingdmgp )	/* (c) 1994 Raizing/8ing (hack?) */

	/* AdvanceMAME new games from ThunderMAME32Plus-v0.106X/ThunderMAME32Plus-2015-09-02-src.7z */
	DRIVER( bgaregga )  /* (c) 1996 Raizing/8ing */
	DRIVER( bgareghk )  /* (c) 1996 Raizing/8ing */
	DRIVER( bgaregnv )  /* (c) 1996 Raizing/8ing */
	DRIVER( bgaregt2 )  /* (c) 1996 Raizing/8ing */
	DRIVER( bgaregcn )  /* (c) 1996 Raizing/8ing */
	DRIVER( bgaregtw )  /* (c) 1996 Raizing/8ing */
	DRIVER( batrid )    /* (c) 1998 Raizing/8ing */
	DRIVER( batridu )   /* (c) 1998 Raizing/8ing */
	DRIVER( batridc )   /* (c) 1998 Raizing/8ing */
	DRIVER( batridj )   /* (c) 1998 Raizing/8ing */
	DRIVER( batridk )   /* (c) 1998 Raizing/8ing */
	DRIVER( batridja )  /* (c) 1998 Raizing/8ing */
	DRIVER( batridta )  /* (c) 1998 Raizing/8ing */
	DRIVER( bkraidu )   /* (c) 1999 8ing */
	DRIVER( bkraiduj )  /* (c) 1999 8ing */
	DRIVER( bkraidj )   /* (c) 1999 8ing */

/*
Toa Plan's board list
(translated from http://www.aianet.ne.jp/~eisetu/rom/rom_toha.html)

Title              ROMno.   Remark(1)   Remark(2)
--------------------------------------------------
Tiger Heli           A47      GX-551
Hishouzame           B02      TP-007
Kyukyoku Tiger       B30      TP-011
Dash Yarou           B45      TP-012
Tatsujin             B65      TP-013B   M6100649A
Zero Wing            O15      TP-015
Horror Story         O16      TP-016
Same!Same!Same!      O17      TP-017
Out Zone                      TP-018
Vimana                        TP-019
Teki Paki            O20      TP-020
Ghox               TP-21      TP-021
Dogyuun                       TP-022
Tatsujin Oh                   TP-024    *1
Fixeight                      TP-026
V-V                           TP-027

*1 There is a doubt this game uses TP-024 board and TP-025 romsets.

   86 Mahjong Sisters                                 Kit 2P 8W+2B     HC    Mahjong TP-
   88 Dash                                            Kit 2P 8W+2B                   TP-
   89 Fire Shark                                      Kit 2P 8W+2B     VC    Shooter TP-017
   89 Twin Hawk                                       Kit 2P 8W+2B     VC    Shooter TP-
   91 Whoopie                                         Kit 2P 8W+2B     HC    Action
   92 Teki Paki                                       Kit 2P                         TP-020
   92 Ghox                                            Kit 2P Paddle+1B VC    Action  TP-021
10/92 Dogyuun                                         Kit 2P 8W+2B     VC    Shooter TP-022
92/93 Knuckle Bash                 Atari Games        Kit 2P 8W+2B     HC    Action  TP-023
10/92 Tatsujin II/Truxton II       Taito              Kit 2P 8W+2B     VC    Shooter TP-024
10/92 Truxton II/Tatsujin II       Taito              Kit 2P 8W+2B     VC    Shooter TP-024
      Pipi & Bipi                                                                    TP-025
   92 Fix Eight                                       Kit 2P 8W+2B     VC    Action  TP-026
12/92 V  -  V (5)/Grind Stormer                       Kit 2P 8W+2B     VC    Shooter TP-027
 1/93 Grind Stormer/V - V (Five)                      Kit 2P 8W+2B     VC    Shooter TP-027
 2/94 Batsugun                                        Kit 2P 8W+2B     VC            TP-
 4/94 Snow Bros. 2                                    Kit 2P 8W+2B     HC    Action  TP-
*/

	/* Cave games */
	/* Cave was formed in 1994 from the ruins of Toaplan, like Raizing was. */
	DRIVER( pwrinst2 )	/* (c) 1994 Atlus */
	DRIVER( pwrins2j )	/* (c) 1994 Atlus */
	DRIVER( plegends )	/* (c) 1994 Atlus */
	DRIVER( plegendj )	/* (c) 1994 Atlus */
	DRIVER( mazinger )	/* (c) 1994 Banpresto (country is in EEPROM) */
	DRIVER( donpachi )	/* (c) 1995 Atlus/Cave */
	DRIVER( donpachj )	/* (c) 1995 Atlus/Cave */
	DRIVER( donpachk )	/* (c) 1995 Atlus/Cave */
	DRIVER( metmqstr )	/* (c) 1995 Banpresto / Pandorabox */
	DRIVER( nmaster )	/* (c) 1995 Banpresto / Pandorabox */
	DRIVER( sailormn )	/* (c) 1995 Banpresto (country is in EEPROM) */
	DRIVER( sailormo )	/* (c) 1995 Banpresto (country is in EEPROM) */
	DRIVER( agallet )	/* (c) 1996 Banpresto / Gazelle (country is in EEPROM) */
	DRIVER( hotdogst )	/* (c) 1996 Marble */
	DRIVER( ddonpach )	/* (c) 1997 Atlus/Cave */
	DRIVER( ddonpchj )	/* (c) 1997 Atlus/Cave */
	DRIVER( dfeveron )	/* (c) 1998 Cave + Nihon System license */
	DRIVER( feversos )	/* (c) 1998 Cave + Nihon System license */
	DRIVER( esprade )	/* (c) 1998 Atlus/Cave */
	DRIVER( espradej )	/* (c) 1998 Atlus/Cave (Japan) */
	DRIVER( espradeo )	/* (c) 1998 Atlus/Cave (Japan) */
	DRIVER( uopoko )	/* (c) 1998 Cave + Jaleco license */
	DRIVER( uopokoj )	/* (c) 1998 Cave + Jaleco license */
	DRIVER( guwange )	/* (c) 1999 Atlus/Cave */
	DRIVER( gaia )		/* (c) 1999 Noise Factory */
	DRIVER( korokoro )	/* (c) 1999 Takumi */

	/* Kyugo games */
	/* Kyugo only made four games: Repulse, Flash Gal, SRD Mission and Air Wolf. */
	/* Gyrodine was made by Crux. Crux was antecedent of Toa Plan, and spin-off from Orca. */
	DRIVER( gyrodine )	/* (c) 1984 Taito Corporation */
	DRIVER( gyrodinc )	/* (c) 1984 Taito Coropration (Crux License) */
	DRIVER( sonofphx )	/* (c) 1985 Associated Overseas MFR */
	DRIVER( repulse )	/* (c) 1985 Sega */
	DRIVER( 99lstwar )	/* (c) 1985 Proma */
	DRIVER( 99lstwra )	/* (c) 1985 Proma */
	DRIVER( 99lstwrk )	/* (c) 1985 Kyugo */
	DRIVER( flashgal )	/* (c) 1985 Sega */
	DRIVER( flashgla )	/* (c) 1985 Sega */
	DRIVER( srdmissn )	/* (c) 1986 Taito Corporation */
	DRIVER( fx )		/* bootleg */
	DRIVER( legend )	/* no copyright notice [1986 Sega/Coreland?] */
	DRIVER( airwolf )	/* (c) 1987 Kyugo */
	DRIVER( skywolf )	/* bootleg */
	DRIVER( skywolf2 )	/* bootleg */

	/* Williams games */
	DRIVER( defender )	/* (c) 1980 */
	DRIVER( defendg )	/* (c) 1980 */
	DRIVER( defendw )	/* (c) 1980 */
	DRIVER( defndjeu )	/* bootleg */
	DRIVER( tornado1 )	/* bootleg */
	DRIVER( tornado2 )	/* bootleg */
	DRIVER( zero )		/* bootleg */
	DRIVER( defcmnd )	/* bootleg */
	DRIVER( defence )	/* bootleg */
	DRIVER( startrkd )	/* bootleg */
	DRIVER( mayday )
	DRIVER( maydaya )
	DRIVER( maydayb )
	DRIVER( colony7 )	/* (c) 1981 Taito */
	DRIVER( colony7a )	/* (c) 1981 Taito */
	DRIVER( jin )		/* Falcon bootleg/hack */
	DRIVER( stargate )	/* (c) 1981 */
	DRIVER( robotron )	/* (c) 1982 */
	DRIVER( robotryo )	/* (c) 1982 */
	DRIVER( joust )		/* (c) 1982 */
	DRIVER( joustr )	/* (c) 1982 */
	DRIVER( joustwr )	/* (c) 1982 */
	DRIVER( bubbles )	/* (c) 1982 */
	DRIVER( bubblesr )	/* (c) 1982 */
	DRIVER( bubblesp )	/* (c) 1982 */
	DRIVER( splat )		/* (c) 1982 */
	DRIVER( sinistar )	/* (c) 1982 */
	DRIVER( sinista1 )	/* (c) 1982 */
	DRIVER( sinista2 )	/* (c) 1982 */
	DRIVER( playball )	/* (c) 1983 */
	DRIVER( blaster )	/* (c) 1983 */
	DRIVER( blast30 )	/* (c) 1983 */
	DRIVER( blastkit )	/* (c) 1983 */
	DRIVER( spdball )	/* (c) 1985 */
	DRIVER( alienar )	/* (c) 1985 Duncan Brown */
	DRIVER( alienaru )	/* (c) 1985 Duncan Brown */
	DRIVER( mysticm )	/* (c) 1983 */
	DRIVER( tshoot )	/* (c) 1984 */
	DRIVER( inferno )	/* (c) 1984 */
	DRIVER( joust2 )	/* (c) 1986 */
	DRIVER( lottofun )	/* (c) 1987 H.A.R. Management */

	/* Capcom games */
	/* The following is a COMPLETE list of the Capcom games up to 1997, as shown on */
	/* their web site. The list is sorted by production date.                       */
	/* A comprehensive list of Capcom games with board info can be found here:      */
	/* http://www.arcadeflyers.com/strider/capcom_list.html                         */
	DRIVER( vulgus )	/*  5/1984 (c) 1984 */
	DRIVER( vulgus2 )	/*  5/1984 (c) 1984 */
	DRIVER( vulgusj )	/*  5/1984 (c) 1984 */
	DRIVER( sonson )	/*  7/1984 (c) 1984 */
	DRIVER( sonsonj )	/*  7/1984 (c) 1984 (Japan) */
	DRIVER( higemaru )	/*  9/1984 (c) 1984 */
	DRIVER( 1942 )		/* 12/1984 (c) 1984 */
	DRIVER( 1942a )		/* 12/1984 (c) 1984 */
	DRIVER( 1942b )		/* 12/1984 (c) 1984 */
	DRIVER( exedexes )	/*  2/1985 (c) 1985 */
	DRIVER( savgbees )	/*  2/1985 (c) 1985 + Memetron license */
	DRIVER( commando )	/*  5/1985 (c) 1985 (World) */
	DRIVER( commandu )	/*  5/1985 (c) 1985 + Data East license (US) */
	DRIVER( commandj )	/*  5/1985 (c) 1985 (Japan) */
	DRIVER( sinvasn )	/* Europe original? */
	DRIVER( sinvasnb )	/* bootleg */
	DRIVER( gng )		/*  9/1985 (c) 1985 */
	DRIVER( gnga )		/*  9/1985 (c) 1985 */
	DRIVER( gngt )		/*  9/1985 (c) 1985 */
	DRIVER( makaimur )	/*  9/1985 (c) 1985 */
	DRIVER( makaimuc )	/*  9/1985 (c) 1985 */
	DRIVER( makaimug )	/*  9/1985 (c) 1985 */
	DRIVER( diamond )	/* (c) 1989 KH Video (NOT A CAPCOM GAME but runs on GnG hardware) */
	DRIVER( gunsmoke )	/* 11/1985 (c) 1985 (World) */
	DRIVER( gunsmoku )	/* 11/1985 (c) 1985 + Romstar (US) */
	DRIVER( gunsmoka )	/* 11/1985 (c) 1985 (US) */
	DRIVER( gunsmokj )	/* 11/1985 (c) 1985 (Japan) */
	DRIVER( sectionz )	/* 12/1985 (c) 1985 */
	DRIVER( sctionza )	/* 12/1985 (c) 1985 */
	DRIVER( trojan )	/*  4/1986 (c) 1986 (US) */
	DRIVER( trojanr )	/*  4/1986 (c) 1986 + Romstar */
	DRIVER( trojanj )	/*  4/1986 (c) 1986 (Japan) */
	DRIVER( srumbler )	/*  9/1986 (c) 1986 */
	DRIVER( srumblr2 )	/*  9/1986 (c) 1986 */
	DRIVER( rushcrsh )	/*  9/1986 (c) 1986 */
	DRIVER( lwings )	/* 11/1986 (c) 1986 */
	DRIVER( lwings2 )	/* 11/1986 (c) 1986 */
	DRIVER( lwingsjp )	/* 11/1986 (c) 1986 */
	DRIVER( sidearms )	/* 12/1986 (c) 1986 (World) */
	DRIVER( sidearmr )	/* 12/1986 (c) 1986 + Romstar license (US) */
	DRIVER( sidearjp )	/* 12/1986 (c) 1986 (Japan) */
	DRIVER( turtship )	/* (c) 1988 Philco (NOT A CAPCOM GAME but runs on modified Sidearms hardware) */
	DRIVER( turtshik )	/* (c) 1988 Philco (NOT A CAPCOM GAME but runs on modified Sidearms hardware) */
	DRIVER( dyger )		/* (c) 1989 Philco (NOT A CAPCOM GAME but runs on modified Sidearms hardware) */
	DRIVER( dygera )	/* (c) 1989 Philco (NOT A CAPCOM GAME but runs on modified Sidearms hardware) */
	DRIVER( twinfalc )	/* (c) 1989 Philko (Poara Enterprises license) (NOT A CAPCOM GAME but runs on modified Sidearms hardware) */
	DRIVER( whizz )		/* (c) 1989 Philco (NOT A CAPCOM GAME but runs on modified Sidearms hardware) */
	DRIVER( avengers )	/*  2/1987 (c) 1987 (US) */
	DRIVER( avenger2 )	/*  2/1987 (c) 1987 (US) */
	DRIVER( buraiken )	/*  2/1987 (c) 1987 (Japan) */
	DRIVER( bionicc )	/*  3/1987 (c) 1987 (US) */
	DRIVER( bionicc2 )	/*  3/1987 (c) 1987 (US) */
	DRIVER( topsecrt )	/*  3/1987 (c) 1987 (Japan) */
	DRIVER( 1943 )		/*  6/1987 (c) 1987 (US) */
	DRIVER( 1943j )		/*  6/1987 (c) 1987 (Japan) */
	DRIVER( blktiger )	/*  8/1987 (c) 1987 (US) */
	DRIVER( bktigerb )	/* bootleg */
	DRIVER( blkdrgon )	/*  8/1987 (c) 1987 (Japan) */
	DRIVER( blkdrgnb )	/* bootleg, hacked to say Black Tiger */
	DRIVER( sf )		/*  8/1987 (c) 1987 (World) */
	DRIVER( sfus )		/*  8/1987 (c) 1987 (US) */
	DRIVER( sfjp )		/*  8/1987 (c) 1987 (Japan) */
	DRIVER( sfp )		/*  8/1987 (c) 1987 */
	DRIVER( tigeroad )	/* 11/1987 (c) 1987 + Romstar (US) */
	DRIVER( toramich )	/* 11/1987 (c) 1987 (Japan) */
	DRIVER( tigerodb )	/* bootleg */
	DRIVER( f1dream )	/*  4/1988 (c) 1988 + Romstar */
	DRIVER( f1dreamb )	/* bootleg */
	DRIVER( 1943kai )	/*  6/1988 (c) 1987 (Japan) */
	DRIVER( lastduel )	/*  7/1988 (c) 1988 (US) */
	DRIVER( lstduela )	/*  7/1988 (c) 1988 (US) */
	DRIVER( lstduelb )	/* bootleg */
	DRIVER( madgear )	/*  2/1989 (c) 1989 (US) */
	DRIVER( madgearj )	/*  2/1989 (c) 1989 (Japan) */
	DRIVER( ledstorm )	/*  2/1989 (c) 1989 (US) */
	/*  3/1989 Dokaben (baseball) - see below among "Mitchell" games */
	/*  8/1989 Dokaben 2 (baseball) - see below among "Mitchell" games */
	/* 10/1989 Capcom Baseball - see below among "Mitchell" games */
	/* 11/1989 Capcom World - see below among "Mitchell" games */
	/*  3/1990 Adventure Quiz 2 Hatena no Dai-Bouken - see below among "Mitchell" games */
	/*  1/1991 Quiz Tonosama no Yabou - see below among "Mitchell" games */
	/*  4/1991 Ashita Tenki ni Naare (golf) - see below among "Mitchell" games */
	/*  5/1991 Ataxx - see below among "Leland" games */
	/*  6/1991 Quiz Sangokushi - see below among "Mitchell" games */
	/* 10/1991 Block Block - see below among "Mitchell" games */
	/*  6/1995 Street Fighter - the Movie - see below among "Incredible Technologies" games */


	/* Capcom CPS1 games */
	DRIVER( forgottn )	/*  7/1988 (c) 1988 (US) */
	DRIVER( lostwrld )	/*  7/1988 (c) 1988 (Japan) */
	DRIVER( ghouls )	/* 12/1988 (c) 1988 (World) */
	DRIVER( ghoulsu )	/* 12/1988 (c) 1988 (US) */
	DRIVER( daimakai )	/* 12/1988 (c) 1988 (Japan) */
	DRIVER( strider )	/*  3/1989 (c) 1989 (not explicitly stated but should be US) */
	DRIVER( stridrua )	/*  3/1989 (c) 1989 (not explicitly stated but should be US) */
	DRIVER( striderj )	/*  3/1989 (c) 1989 */
	DRIVER( stridrja )	/*  3/1989 (c) 1989 */
	DRIVER( dynwar )	/*  4/1989 (c) 1989 (World) */
	DRIVER( dynwarj )	/*  4/1989 (c) 1989 (Japan) */
	DRIVER( willow )	/*  6/1989 (c) 1989 (US) */
	DRIVER( willowj )	/*  6/1989 (c) 1989 (Japan) */
	DRIVER( willowje )	/*  6/1989 (c) 1989 (Japan) */
	DRIVER( unsquad )	/*  8/1989 (c) 1989 */
	DRIVER( area88 )	/*  8/1989 (c) 1989 */
	DRIVER( ffight )	/* 12/1989 (c) (World) */
	DRIVER( ffightu )	/* 12/1989 (c) (US)    */
	DRIVER( ffightua )	/* 12/1989 (c) (US)    */
	DRIVER( ffightj )	/* 12/1989 (c) (Japan) */
	DRIVER( ffightj1 )	/* 12/1989 (c) (Japan) */
	DRIVER( 1941 )		/*  2/1990 (c) 1990 (World) */
	DRIVER( 1941j )		/*  2/1990 (c) 1990 (Japan) */
	DRIVER( mercs )		/* 02/03/1990 (c) 1990 (World) */
	DRIVER( mercsu )	/* 02/03/1990 (c) 1990 (US)    */
	DRIVER( mercsua )	/* 08/06/1990 (c) 1990 (US)    */
	DRIVER( mercsj )	/* 02/03/1990 (c) 1990 (Japan) */
	DRIVER( mtwins )	/* 19/06/1990 (c) 1990 (World) */
	DRIVER( chikij )	/* 19/06/1990 (c) 1990 (Japan) */
	DRIVER( msword )	/* 25/07/1990 (c) 1990 (World) */
	DRIVER( mswordr1 )	/* 23/06/1990 (c) 1990 (World) */
	DRIVER( mswordu )	/* 25/07/1990 (c) 1990 (US)    */
	DRIVER( mswordj )	/* 23/06/1990 (c) 1990 (Japan) */
	DRIVER( cawing )	/* 12/10/1990 (c) 1990 (World) */
	DRIVER( cawingr1 )	/* 09/10/1990 (c) 1990 (World) */
	DRIVER( cawingu )	/* 12/10/1990 (c) 1990 (US) */
	DRIVER( cawingj )	/* 12/10/1990 (c) 1990 (Japan) */
	DRIVER( nemo )		/* 30/11/1990 (c) 1990 (World) */
	DRIVER( nemoj )		/* 20/11/1990 (c) 1990 (Japan) */
	DRIVER( sf2 )		/* 22/05/1991 (c) 1991 (World) */
	DRIVER( sf2eb )		/* 14/02/1991 (c) 1991 (World) */
	DRIVER( sf2ua )		/* 06/02/1991 (c) 1991 (US)    */
	DRIVER( sf2ub )		/* 14/02/1991 (c) 1991 (US)    */
	DRIVER( sf2ud )		/* 18/03/1991 (c) 1991 (US)    */
	DRIVER( sf2ue )		/* 28/02/1991 (c) 1991 (US)    */
	DRIVER( sf2uf )		/* 11/04/1991 (c) 1991 (US)    */
	DRIVER( sf2ui )		/* 22/05/1991 (c) 1991 (US)    */
	DRIVER( sf2uk )		/* 01/11/1991 (c) 1991 (US)    */
	DRIVER( sf2j )		/* 10/12/1991 (c) 1991 (Japan) */
	DRIVER( sf2ja )		/* 14/02/1991 (c) 1991 (Japan) */
	DRIVER( sf2jc )		/* 06/03/1991 (c) 1991 (Japan) */
	DRIVER( 3wonders )	/* 20/05/1991 (c) 1991 (World) */
	DRIVER( 3wonderu )	/* 20/05/1991 (c) 1991 (US)    */
	DRIVER( wonder3 )	/* 20/05/1991 (c) 1991 (Japan) */
	DRIVER( kod )		/* 11/07/1991 (c) 1991 (World) */
	DRIVER( kodu )		/* 10/09/1991 (c) 1991 (US)    */
	DRIVER( kodj )		/* 05/08/1991 (c) 1991 (Japan) */
	DRIVER( kodb )		/* bootleg */
	DRIVER( captcomm )	/* 14/10/1991 (c) 1991 (World) */
	DRIVER( captcomu )	/* 28/ 9/1991 (c) 1991 (US)    */
	DRIVER( captcomj )	/* 02/12/1991 (c) 1991 (Japan) */
	DRIVER( knights )	/* 27/11/1991 (c) 1991 (World) */
	DRIVER( knightsu )	/* 27/11/1991 (c) 1991 (US)    */
	DRIVER( knightsj )	/* 27/11/1991 (c) 1991 (Japan) */
	DRIVER( sf2ce )		/* 13/03/1992 (c) 1992 (World) */
	DRIVER( sf2ceua )	/* 13/03/1992 (c) 1992 (US)    */
	DRIVER( sf2ceub )	/* 13/05/1992 (c) 1992 (US)    */
	DRIVER( sf2ceuc )	/* 03/08/1992 (c) 1992 (US)    */
	DRIVER( sf2cej )	/* 13/05/1992 (c) 1992 (Japan) */
	DRIVER( sf2rb )		/* hack */
	DRIVER( sf2rb2 )	/* hack */
	DRIVER( sf2red )	/* hack */
	DRIVER( sf2v004 )	/* hack */
	DRIVER( sf2accp2 )	/* hack */
	DRIVER( sf2m1 )		/* hack */
	DRIVER( sf2m2 )		/* hack */
	DRIVER( sf2m3 )		/* hack */
	DRIVER( sf2m4 )		/* hack */
	DRIVER( sf2m5 )		/* hack */
	DRIVER( sf2m6 )		/* hack */
	DRIVER( sf2m7 )		/* hack */
	DRIVER( sf2yyc )	/* hack */
	DRIVER( sf2koryu )	/* hack */
	DRIVER( varth )		/* 14/07/1992 (c) 1992 (World) */
	DRIVER( varthr1 )	/* 12/06/1992 (c) 1992 (World) */
	DRIVER( varthu )	/* 12/06/1992 (c) 1992 (US) */
	DRIVER( varthj )	/* 14/07/1992 (c) 1992 (Japan) */
	DRIVER( cworld2j )	/* 11/06/1992 (QUIZ 5) (c) 1992 (Japan) */
	DRIVER( wof )		/* 02/10/1992 (c) 1992 (World) (CPS1 + QSound) */
	DRIVER( wofa )		/* 05/10/1992 (c) 1992 (Asia)  (CPS1 + QSound) */
	DRIVER( wofu )		/* 31/10/1992 (c) 1992 (US) (CPS1 + QSound) */
	DRIVER( wofj )		/* 31/10/1992 (c) 1992 (Japan) (CPS1 + QSound) */
	DRIVER( sf2hf )		/* 09/12/1992 (c) 1992 (World) */
	DRIVER( sf2t )		/* 09/12/1992 (c) 1992 (US)    */
	DRIVER( sf2tj )		/* 09/12/1992 (c) 1992 (Japan) */
	DRIVER( dino )		/* 01/02/1993 (c) 1993 (World) (CPS1 + QSound) */
	DRIVER( dinou )		/* 01/02/1993 (c) 1993 (US)    (CPS1 + QSound) */
	DRIVER( dinoj )		/* 01/02/1993 (c) 1993 (Japan) (CPS1 + QSound) */
	DRIVER( punisher )	/* 22/04/1993 (c) 1993 (World) (CPS1 + QSound) */
	DRIVER( punishru )	/* 22/04/1993 (c) 1993 (US)    (CPS1 + QSound) */
	DRIVER( punishrj )	/* 22/04/1993 (c) 1993 (Japan) (CPS1 + QSound) */
	DRIVER( slammast )	/* 13/07/1993 (c) 1993 (World) (CPS1 + QSound) */
	DRIVER( slammasu )	/* 13/07/1993 (c) 1993 (US)    (CPS1 + QSound) */
	DRIVER( mbomberj )	/* 13/07/1993 (c) 1993 (Japan) (CPS1 + QSound) */
	DRIVER( mbombrd )	/* 06/12/1993 (c) 1993 (World) (CPS1 + QSound) */
	DRIVER( mbombrdj )	/* 06/12/1993 (c) 1993 (Japan) (CPS1 + QSound) */
	DRIVER( pnickj )	/* 08/06/1994 (c) 1994 Compile + Capcom license (Japan) not listed on Capcom's site */
	DRIVER( qad )		/* 01/07/1992 (c) 1992 (US)    */
	DRIVER( qadj )		/* 21/09/1994 (c) 1994 (Japan) */
	DRIVER( qtono2 )	/* 23/01/1995 (c) 1995 (Japan) */
	DRIVER( pang3 )		/* 11/05/1995 (c) 1995 Mitchell (Euro) not listed on Capcom's site */
	DRIVER( pang3j )	/* 11/05/1995 (c) 1995 Mitchell (Japan) not listed on Capcom's site */
	DRIVER( megaman )	/* 06/10/1995 (c) 1995 (Asia)  */
	DRIVER( rockmanj )	/* 22/09/1995 (c) 1995 (Japan) */

	DRIVER( fcrash )	/* bootleg of Final Fight */

	/* Capcom CPS2 games */
	/* list completed by CPS2Shock */
	/* http://cps2shock.retrogames.com */
	DRIVER( ssf2 )		/* 11/09/1993 (c) 1993 (World) */
	DRIVER( ssf2u )		/* 11/09/1993 (c) 1993 (US) */
	DRIVER( ssf2a )		/* 05/10/1993 (c) 1993 (Asia) */
	DRIVER( ssf2ar1 )	/* 14/09/1993 (c) 1993 (Asia) */
	DRIVER( ssf2j )		/* 05/10/1993 (c) 1993 (Japan) */
	DRIVER( ssf2jr1 )	/* 11/09/1993 (c) 1993 (Japan) */
	DRIVER( ssf2jr2 )	/* 10/09/1993 (c) 1993 (Japan) */
	DRIVER( ssf2tb )	/* 11/19/1993 (c) 1993 (World) */
	DRIVER( ssf2tbr1 )	/* 11/09/1993 (c) 1993 (World) */
	DRIVER( ssf2tbj )	/* 11/09/1993 (c) 1993 (Japan) */
	DRIVER( ecofghtr )	/* 03/12/1993 (c) 1993 (World) */
	DRIVER( ecofghtu )	/* 03/12/1993 (c) 1993 (US) */
	DRIVER( ecofghta )	/* 03/12/1993 (c) 1993 (Asia) */
	DRIVER( uecology ) 	/* 03/12/1993 (c) 1993 (Japan) */
	DRIVER( ddtod )		/* 12/04/1994 (c) 1993 (Euro) */
	DRIVER( ddtodr1 )	/* 13/01/1994 (c) 1993 (Euro) */
	DRIVER( ddtodu )	/* 25/01/1994 (c) 1993 (US) */
	DRIVER( ddtodur1 )	/* 13/01/1994 (c) 1993 (US) */
	DRIVER( ddtodj )	/* 25/01/1994 (c) 1993 (Japan) */
	DRIVER( ddtodjr1 )	/* 13/01/1994 (c) 1993 (Japan) */
	DRIVER( ddtoda )	/* 13/01/1994 (c) 1993 (Asia) */
	DRIVER( ddtodh )	/* 25/01/1994 (c) 1993 (Hispanic) */
	DRIVER( ssf2t )		/* 23/02/1994 (c) 1994 (World) */
	DRIVER( ssf2tu )	/* 23/03/1994 (c) 1994 (US) */
	DRIVER( ssf2tur1 )	/* 23/02/1994 (c) 1994 (US) */
	DRIVER( ssf2ta )	/* 23/02/1994 (c) 1994 (Asia) */
	DRIVER( ssf2xj )	/* 23/02/1994 (c) 1994 (Japan) */
	DRIVER( avsp )		/* 20/05/1994 (c) 1994 (Euro) */
	DRIVER( avspu )		/* 20/05/1994 (c) 1994 (US) */
	DRIVER( avspj )		/* 20/05/1994 (c) 1994 (Japan) */
	DRIVER( avspa )		/* 20/05/1994 (c) 1994 (Asia) */
	DRIVER( avsph )		/* 20/05/1994 (c) 1994 (Hispanic) */
						/*    06/1994? Galum Pa! (not listed on Capcom's site) */
	DRIVER( dstlk )		/* 05/07/1994 (c) 1994 (Euro) */
	DRIVER( dstlku )	/* 18/08/1994 (c) 1994 (US) */
	DRIVER( dstlkur1 )	/* 05/07/1994 (c) 1994 (US) */
	DRIVER( dstlka )	/* 05/07/1994 (c) 1994 (Asia) */
	DRIVER( vampj )		/* 05/07/1994 (c) 1994 (Japan) */
	DRIVER( vampja )	/* 05/07/1994 (c) 1994 (Japan) */
	DRIVER( vampjr1 )	/* 30/06/1994 (c) 1994 (Japan) */
	DRIVER( ringdest )	/* 02/09/1994 (c) 1994 (Euro) */
	DRIVER( smbomb )	/* 31/08/1994 (c) 1994 (Japan) */
	DRIVER( smbombr1 )	/* 08/08/1994 (c) 1994 (Japan) */
	DRIVER( armwar )	/* 24/10/1994 (c) 1994 (Euro) */
	DRIVER( armwarr1 )	/* 11/10/1994 (c) 1994 (Euro) */
	DRIVER( armwaru )	/* 24/10/1994 (c) 1994 (US) */
	DRIVER( pgear )		/* 24/10/1994 (c) 1994 (Japan) */
	DRIVER( pgearr1 )	/* 16/09/1994 (c) 1994 (Japan) */
	DRIVER( armwara )	/* 20/09/1994 (c) 1994 (Asia) */
	DRIVER( xmcota )	/* 05/01/1995 (c) 1994 (Euro) */
	DRIVER( xmcotau )	/* 05/01/1995 (c) 1994 (US) */
	DRIVER( xmcotah )	/* 31/03/1995 (c) 1994 (Hispanic) */
	DRIVER( xmcotaj )	/* 19/12/1994 (c) 1994 (Japan) */
	DRIVER( xmcotaj1 )	/* 17/12/1994 (c) 1994 (Japan) */
	DRIVER( xmcotaa )	/* 17/12/1994 (c) 1994 (Asia) */
	DRIVER( xmcotajr )	/* 08/12/1994 (c) 1994 (Japan Rent) */
	DRIVER( nwarr )		/* 06/04/1995 (c) 1995 (US) */
	DRIVER( nwarrh )	/* 03/04/1995 (c) 1995 (Hispanic) */
	DRIVER( nwarrb )	/* 03/04/1995 (c) 1995 (Brazil) */
	DRIVER( vhuntj )	/* 02/03/1995 (c) 1995 (Japan) */
	DRIVER( vhuntjr1 )	/* 07/03/1995 (c) 1995 (Japan) */
	DRIVER( vhuntjr2 )	/* 02/03/1995 (c) 1995 (Japan) */
	DRIVER( cybots )	/* 24/04/1995 (c) 1995 (Euro) */
	DRIVER( cybotsu )	/* 24/04/1995 (c) 1995 (US) */
	DRIVER( cybotsj )	/* 20/04/1995 (c) 1995 (Japan) */
	DRIVER( sfa )		/* 27/07/1995 (c) 1995 (Euro) */
	DRIVER( sfar1 )		/* 18/07/1995 (c) 1995 (Euro) */
	DRIVER( sfar2 )		/* 27/06/1995 (c) 1995 (Euro) */
	DRIVER( sfar3 )		/* 05/06/1995 (c) 1995 (Euro) */
	DRIVER( sfau )		/* 27/06/1995 (c) 1995 (US) */
	DRIVER( sfza )		/* 27/06/1995 (c) 1995 (Asia) */
	DRIVER( sfzj )		/* 27/07/1995 (c) 1995 (Japan) */
	DRIVER( sfzjr1 )	/* 27/06/1995 (c) 1995 (Japan) */
	DRIVER( sfzjr2 )	/* 05/06/1995 (c) 1995 (Japan) */
	DRIVER( sfzh )		/* 27/06/1995 (c) 1995 (Hispanic) */
	DRIVER( sfzb )		/* 09/11/1995 (c) 1995 (Brazil) */
	DRIVER( sfzbr1 )	/* 27/07/1995 (c) 1995 (Brazil) */
	DRIVER( rckmanj )	/* 22/09/1995 (c) 1995 (Japan) */
	DRIVER( msh )		/* 24/10/1995 (c) 1995 (Euro) */
	DRIVER( mshu )		/* 24/10/1995 (c) 1995 (US) */
	DRIVER( mshj )		/* 17/11/1995 (c) 1995 (Japan) */
	DRIVER( mshjr1 )	/* 24/10/1995 (c) 1995 (Japan) */
	DRIVER( msha )		/* 24/10/1995 (c) 1995 (Asia) */
	DRIVER( mshh )		/* 17/11/1995 (c) 1996 (Hispanic) */
	DRIVER( mshb )		/* 17/11/1995 (c) 1996 (Brazil) */
	DRIVER( 19xx )		/* 07/12/1995 (c) 1996 (US) */
	DRIVER( 19xxj )		/* 25/12/1995 (c) 1996 (Japan) */
	DRIVER( 19xxjr1 )	/* 07/12/1995 (c) 1996 (Japan) */
	DRIVER( 19xxa )		/* 07/12/1995 (c) 1996 (Asia) */
	DRIVER( 19xxh )		/* 18/12/1995 (c) 1996 (Hispanic) */
	DRIVER( ddsom )		/* 19/06/1996 (c) 1996 (Euro) */
	DRIVER( ddsomr1 )	/* 23/02/1996 (c) 1996 (Euro) */
	DRIVER( ddsomr2 )	/* 09/02/1996 (c) 1996 (Euro) */
	DRIVER( ddsomu )	/* 19/06/1996 (c) 1996 (US) */
	DRIVER( ddsomur1 )	/* 09/02/1996 (c) 1996 (US) */
	DRIVER( ddsomjr1 )	/* 06/02/1996 (c) 1996 (Japan) */
	DRIVER( ddsomj )	/* 19/06/1996 (c) 1996 (Japan) */
	DRIVER( ddsoma )	/* 19/06/1996 (c) 1996 (Asia) */
	DRIVER( sfa2 )		/* 06/03/1996 (c) 1996 (US) */
	DRIVER( sfz2j )		/* 27/02/1996 (c) 1996 (Japan) */
	DRIVER( sfz2a )		/* 27/02/1996 (c) 1996 (Asia) */
	DRIVER( sfz2b )		/* 31/05/1996 (c) 1996 (Brazil) */
	DRIVER( sfz2br1 )	/* 04/03/1996 (c) 1996 (Brazil) */
	DRIVER( sfz2h )		/* 04/03/1996 (c) 1996 (Hispanic) */
	DRIVER( sfz2n )		/* 29/02/1996 (c) 1996 (Oceania) */
	DRIVER( spf2t )		/* 20/06/1996 (c) 1996 (US) */
	DRIVER( spf2xj )	/* 31/05/1996 (c) 1996 (Japan) */
	DRIVER( spf2ta )	/* 29/05/1996 (c) 1996 (Asia) */
	DRIVER( qndream )	/* 26/06/1996 (c) 1996 (Japan) */
	DRIVER( megaman2 )	/* 08/07/1996 (c) 1996 (US) */
	DRIVER( megamn2a )	/* 08/07/1996 (c) 1996 (Asia) */
	DRIVER( rckman2j )	/* 08/07/1996 (c) 1996 (Japan) */
	DRIVER( sfz2aj )	/* 05/08/1996 (c) 1996 (Japan) */
	DRIVER( sfz2ah )	/* 13/08/1996 (c) 1996 (Hispanic) */
	DRIVER( sfz2ab )	/* 13/08/1996 (c) 1996 (Brazil) */
	DRIVER( sfz2aa )	/* 26/08/1996 (c) 1996 (Asia) */
	DRIVER( xmvsf )		/* 04/10/1996 (c) 1996 (Euro) */
	DRIVER( xmvsfr1 )	/* 10/09/1996 (c) 1996 (Euro) */
	DRIVER( xmvsfu )	/* 23/10/1996 (c) 1996 (US) */
	DRIVER( xmvsfur1 )	/* 04/10/1996 (c) 1996 (US) */
	DRIVER( xmvsfj )	/* 04/10/1996 (c) 1996 (Japan) */
	DRIVER( xmvsfjr1 )	/* 10/09/1996 (c) 1996 (Japan) */
	DRIVER( xmvsfjr2 )	/* 09/09/1996 (c) 1996 (Japan) */
	DRIVER( xmvsfa )	/* 23/10/1996 (c) 1996 (Asia) */
	DRIVER( xmvsfar1 )	/* 19/09/1996 (c) 1996 (Asia) */
	DRIVER( xmvsfh )	/* 04/10/1996 (c) 1996 (Hispanic) */
	DRIVER( xmvsfb )	/* 23/10/1996 (c) 1996 (Brazil) */
	DRIVER( batcir )	/* 19/03/1997 (c) 1997 (Euro) */
	DRIVER( batcirj )	/* 19/03/1997 (c) 1997 (Japan) */
	DRIVER( batcira )	/* 19/03/1997 (c) 1997 (Asia) */
	DRIVER( vsav )		/* 19/05/1997 (c) 1997 (Euro) */
	DRIVER( vsavu )		/* 19/05/1997 (c) 1997 (US) */
	DRIVER( vsavj )		/* 19/05/1997 (c) 1997 (Japan) */
	DRIVER( vsava )		/* 19/05/1997 (c) 1997 (Asia) */
	DRIVER( vsavh )		/* 19/05/1997 (c) 1997 (Hispanic) */
	DRIVER( mshvsf )	/* 25/06/1997 (c) 1997 (Euro) */
	DRIVER( mshvsfu )	/* 27/08/1997 (c) 1997 (US) */
	DRIVER( mshvsfu1 )	/* 25/06/1997 (c) 1997 (US) */
	DRIVER( mshvsfj )	/* 07/07/1997 (c) 1997 (Japan) */
	DRIVER( mshvsfj1 )	/* 02/07/1997 (c) 1997 (Japan) */
	DRIVER( mshvsfj2 )	/* 25/06/1997 (c) 1997 (Japan) */
	DRIVER( mshvsfh )	/* 25/06/1997 (c) 1997 (Hispanic) */
	DRIVER( mshvsfa )	/* 25/06/1997 (c) 1997 (Asia) */
	DRIVER( mshvsfa1 )	/* 20/06/1997 (c) 1997 (Asia) */
	DRIVER( mshvsfb )	/* 27/08/1997 (c) 1997 (Brazil) */
	DRIVER( mshvsfb1 )	/* 25/06/1997 (c) 1997 (Brazil) */
	DRIVER( csclub )	/* 22/07/1997 (c) 1997 (Euro) */
	DRIVER( csclubj )	/* 22/07/1997 (c) 1997 (Japan) */
	DRIVER( cscluba )	/* 22/07/1997 (c) 1997 (Asia) */
	DRIVER( csclubh )	/* 22/07/1997 (c) 1997 (Hispanic) */
	DRIVER( sgemf )		/* 04/09/1997 (c) 1997 (US) */
	DRIVER( pfghtj )	/* 04/09/1997 (c) 1997 (Japan) */
	DRIVER( sgemfa )	/* 04/09/1997 (c) 1997 (Asia) */
	DRIVER( sgemfh )	/* 04/09/1997 (c) 1997 (Hispanic) */
	DRIVER( vhunt2r1 )	/* 13/09/1997 (c) 1997 (Japan) */
	DRIVER( vhunt2 )	/* 29/09/1997 (c) 1997 (Japan) */
	DRIVER( vsav2 )		/* 13/09/1997 (c) 1997 (Japan) */
	DRIVER( mvsc )		/* 12/01/1998 (c) 1998 (Euro) */
	DRIVER( mvscu )		/* 23/01/1998 (c) 1998 (US) */
	DRIVER( mvscj )		/* 23/01/1998 (c) 1998 (Japan) */
	DRIVER( mvscjr1 )	/* 12/01/1998 (c) 1998 (Japan) */
	DRIVER( mvsca )		/* 23/01/1998 (c) 1998 (Asia) */
	DRIVER( mvscar1 )	/* 12/01/1998 (c) 1998 (Asia) */
	DRIVER( mvsch )		/* 23/01/1998 (c) 1998 (Hispanic) */
	DRIVER( mvscb )		/* 23/01/1998 (c) 1998 (Brazil) */
	DRIVER( sfa3 )		/* 04/09/1998 (c) 1998 (US) */
	DRIVER( sfa3r1 )	/* 29/06/1998 (c) 1998 (US) */
	DRIVER( sfa3b )		/* 29/06/1998 (c) 1998 (Brazil) */
	DRIVER( sfz3j )		/* 04/09/1998 (c) 1998 (Japan) */
	DRIVER( sfz3jr1 )	/* 27/07/1998 (c) 1998 (Japan) */
	DRIVER( sfz3jr2 )	/* 29/06/1998 (c) 1998 (Japan) */
	DRIVER( sfz3a )		/* 04/09/1998 (c) 1998 (Asia) */
	DRIVER( sfz3ar1 )	/* 01/07/1998 (c) 1998 (Asia) */
	DRIVER( gigawing )	/* 22/02/1999 (c) 1999 Takumi (US) */
	DRIVER( gwingj )	/* 23/02/1999 (c) 1999 Takumi (Japan) */
	DRIVER( gwinga )	/* 22/02/1999 (c) 1999 Takumi (Asia) */
						/* 27/05/1999 Jyangokushi: Haoh no Saihai (Japan) */
	DRIVER( dimahoo )	/* 21/01/2000 (c) 2000 Eighting/Raizing (US) */
	DRIVER( gmahou )	/* 21/01/2000 (c) 2000 Eighting/Raizing (Japan) */
	DRIVER( mmatrix )	/* 12/04/2000 (c) 2000 Takumi (US) */
	DRIVER( mmatrixj )	/* 12/04/2000 (c) 2000 Takumi (Japan) */
	DRIVER( 1944 )		/* 20/06/2000 (c) 2000 Eighting/Raizing (US) */
	DRIVER( 1944j )		/* 20/06/2000 (c) 2000 Eighting/Raizing (Japan) */
	DRIVER( mpangj )	/* 11/10/2000 (c) 2000 Mitchell */
	DRIVER( progear )	/* 17/01/2001 (c) 2001 Cave (US) */
	DRIVER( progearj )	/* 17/01/2001 (c) 2001 Cave (Japan) */
	DRIVER( progeara )	/* 17/01/2001 (c) 2001 Cave (Asia) */
	DRIVER( pzloop2j )	/* 05/02/2001 (c) 2001 Mitchell (Japan) */
	DRIVER( choko )		/* 20/08/2001 (c) 2001 Mitchell (Japan) */
						/* 22/12/2003 Hyper Street Fighter 2 (Capcom, Japan) */
						/* 02/02/2004 Hyper Street Fighter 2 (Capcom, Asia) */

	/* Capcom CPS3 games */
	DRIVER( warzard )	/* 10/1996 Warzard */
	DRIVER( sfiii )		/* 02/1997 Street Fighter III - New Generation */
	DRIVER( jojo )		/* ??/???? Jojo's Venture */

	/* AdvanceMAME new games from ThunderMAME32Plus-v0.106X/ThunderMAME32Plus-2015-09-02-src.7z */
	DRIVER( jojoalt )		/* ??/???? Jojo's Venture */

	DRIVER( jojoba )	/* ??/???? Jojo's Bizarre Adventure */
	DRIVER( jojobaa )	/* ??/???? Jojo's Bizarre Adventure */
	DRIVER( sfiii2 )	/* ??/???? Street Fighter 3: Second Impact ~giant attack~ */
	DRIVER( sfiii3 )	/* ??/???? Street Fighter 3: Third Strike ~fight to the finish~ */

	/* AdvanceMAME new games from ThunderMAME32Plus-v0.106X/ThunderMAME32Plus-2015-09-02-src.7z */
	DRIVER( sfiii3a )	/* ??/???? Street Fighter 3: Third Strike ~fight to the finish~ */

	/* Capcom ZN1 */
	DRIVER( cpzn1 )
	DRIVER( ts2 )		/* Battle Arena Toshinden 2 (USA 951124) */
	DRIVER( ts2j )		/* Battle Arena Toshinden 2 (JAPAN 951124) */
	DRIVER( starglad )	/* Star Gladiator (USA 960627) */
	DRIVER( sfex )		/* Street Fighter EX (USA 961219) */
	DRIVER( sfexa )		/* Street Fighter EX (ASIA 961219) */
	DRIVER( sfexj )		/* Street Fighter EX (JAPAN 961130) */
	DRIVER( glpracr )	/* Gallop Racer (JAPAN Ver 9.01.12) */
	DRIVER( sfexp )		/* Street Fighter EX Plus (USA 970407) */
	DRIVER( sfexpu1 )	/* Street Fighter EX Plus (USA 970311) */
	DRIVER( sfexpj )	/* Street Fighter EX Plus (JAPAN 970311) */

	/* Capcom ZN2 */
	DRIVER( cpzn2 )
	DRIVER( rvschool )	/* Rival Schools (USA 971117) */
	DRIVER( rvschola )	/* Rival Schools (ASIA 971117) */
	DRIVER( jgakuen )	/* Justice Gakuen (JAPAN 971117) */
	DRIVER( sfex2 )		/* Street Fighter EX 2 (USA 980526) */
	DRIVER( sfex2j )	/* Street Fighter EX 2 (JAPAN 980312) */
	DRIVER( sfex2a )	/* Street Fighter EX 2 (ASIA 980312) */
	DRIVER( plsmaswd )	/* Plasma Sword (USA 980316) */
	DRIVER( stargld2 )	/* Star Gladiator 2 (JAPAN 980316) */
	DRIVER( plsmswda )	/* Plasma Sword (ASIA 980316) */
	DRIVER( tgmj )		/* Tetris The Grand Master (JAPAN 980710) */
	DRIVER( techromn )	/* Tech Romancer (EURO 980914) */
	DRIVER( techromu )	/* Tech Romancer (USA 980914) */
	DRIVER( kikaioh )	/* Kikaioh (JAPAN 980914) */
	DRIVER( sfex2p )	/* Street Fighter EX 2 Plus (USA 990611) */
	DRIVER( sfex2pa )	/* Street Fighter EX 2 Plus (ASIA 990611) */
	DRIVER( sfex2pj )	/* Street Fighter EX 2 Plus (JAPAN 990611) */
	DRIVER( strider2 )	/* Strider 2 (USA 991213) */
	DRIVER( stridr2a )	/* Strider 2 (ASIA 991213) */
	DRIVER( shiryu2 )	/* Strider Hiryu 2 (JAPAN 991213) */

	/* Tecmo ZN1 */
	DRIVER( tps )
	DRIVER( glpracr2 )	/* Gallop Racer 2 (USA) */
	DRIVER( glprac2j )	/* Gallop Racer 2 (JAPAN) */
	DRIVER( glprac2l )	/* Gallop Racer 2 Link HW (JAPAN) */
	DRIVER( doapp )		/* Dead Or Alive ++ (JAPAN) */
	DRIVER( cbaj )		/* Cool Boaders Arcade Jam */
	DRIVER( shngmtkb )	/* Shanghai Matekibuyuu */
	DRIVER( tondemo )	/* Tondemo Crisis (JAPAN) */
	DRIVER( glpracr3 )	/* Gallop Racer 3 (JAPAN) */
	DRIVER( flamegun )	/* Flame Gunner (USA) */
	DRIVER( flameguj )	/* Flame Gunner (JAPAN) */
	DRIVER( tecmowcm )	/* Tecmo World Cup Millenium (JAPAN) */
	DRIVER( mfjump )	/* Monster Farm Jump (JAPAN) */

	/* Atlus ZN1 */
	DRIVER( atluspsx )
	DRIVER( hvnsgate )	/* Heaven's Gate (JAPAN) */

	/* PS Arcade 95 */
	DRIVER( psarc95 )
	DRIVER( beastrzr )	/* Beastorizer (USA) */
	DRIVER( beastrzb )	/* Beastorizer (USA Bootleg) */
	DRIVER( bldyror2 )	/* Bloody Roar 2 (JAPAN) */
	DRIVER( brvblade )	/* Brave Blade (JAPAN) */

	/* Atari PSX */
	DRIVER( atpsx )
	DRIVER( primrag2 )	/* Primal Rage 2 */

	/* Acclaim PSX */
	DRIVER( acpsx )
	DRIVER( nbajamex )	/* NBA Jam Extreme */
	DRIVER( jdredd )	/* Judge Dredd (Rev C) */
	DRIVER( jdreddb )	/* Judge Dredd (Rev B) */

	/* Video System ZN1 */
	DRIVER( sncwgltd )	/* Sonic Wings Limited (JAPAN) */

	/* Taito FX1a */
	DRIVER( taitofx1 )
	DRIVER( sfchamp )	/* Super Football Champ (Ver 2.5J) */
	DRIVER( sfchampj )	/* Super Football Champ (Ver 2.4J) */
	DRIVER( psyforce )	/* Psychic Force (Ver 2.4O) */
	DRIVER( psyforcj )	/* Psychic Force (Ver 2.4J) */
	DRIVER( psyfrcex )	/* Psychic Force EX (Ver 2.0J) */
	DRIVER( mgcldate )	/* Magical Date (JAPAN Ver 2.02J) */
	DRIVER( mgcldtex )	/* Magical Date EX (JAPAN Ver 2.01J) */

	/* Taito FX1b */
	DRIVER( raystorm )	/* Ray Storm (Ver 2.06A) */
	DRIVER( raystorj )	/* Ray Storm (Ver 2.05J) */
	DRIVER( ftimpact )	/* Fighters' Impact (Ver 2.02O) */
	DRIVER( ftimpcta )	/* Fighters' Impact A (Ver 2.00J) */
	DRIVER( gdarius )	/* G-Darius (Ver 2.01J) */
	DRIVER( gdariusb )	/* G-Darius (Ver 2.02A) */
	DRIVER( gdarius2 )	/* G-Darius Ver.2 (Ver 2.03J) */

	/* Taito GNET */
	DRIVER( taitogn )

	/* Namco System 11 */
	DRIVER( tekken )	/* Tekken (TE4/VER.C) */
	DRIVER( tekkena )	/* Tekken (TE2/VER.C) */
	DRIVER( tekkenb )	/* Tekken (TE2/VER.B) */
	DRIVER( tekkenc )	/* Tekken (TE1/VER.B) */
	DRIVER( tekken2 )	/* Tekken 2 Ver.B (TES3/VER.B) */
	DRIVER( tekken2a )	/* Tekken 2 Ver.B (TES2/VER.B) */
	DRIVER( tekken2b )	/* Tekken 2 (TES2/VER.A) */
	DRIVER( souledge )	/* Soul Edge Ver. II (SO4/VER.C) */
	DRIVER( souledga )	/* Soul Edge Ver. II (SO3/VER.C) */
	DRIVER( souledgb )	/* Soul Edge (SO3/VER.A) */
	DRIVER( souledgc )	/* Soul Edge (SO1/VER.A) */
	DRIVER( dunkmnia )	/* Dunk Mania (DM1/VER.C) */
	DRIVER( dunkmnic )	/* Dunk Mania (DM1/VER.C) */
	DRIVER( xevi3dg )	/* Xevious 3D/G (XV31/VER.A) */
	DRIVER( primglex )	/* Prime Goal EX (PG1/VER.A) */
	DRIVER( danceyes )	/* Dancing Eyes (DC1/VER.A) */
	DRIVER( starswep )	/* Star Sweep (STP1/VER.A) */
	DRIVER( myangel3 )	/* Kosodate Quiz My Angel 3 (KQT1/VER.A) */
	DRIVER( ptblnk2a )	/* Point Blank 2 (GNB3/VER.A) */

	/* Namco System 12 */
	DRIVER( tekken3 )	/* Tekken 3 (TET1/VER.E1) */
	DRIVER( tekken3a )	/* Tekken 3 (TET2/VER.B) */
	DRIVER( tekken3b )	/* Tekken 3 (TET2/VER.A) */
	DRIVER( lbgrande )	/* Libero Grande (LG2/VER.A) */
	DRIVER( soulclbr )	/* Soul Calibur (SOC14/VER.C) */
	DRIVER( soulclba )	/* Soul Calibur (SOC11/VER.C) */
	DRIVER( soulclbb )	/* Soul Calibur (SOC11/VER.B) */
	DRIVER( soulclbc )	/* Soul Calibur (SOC11/VER.A2) */
	DRIVER( ehrgeiz )	/* Ehrgeiz (EG3/VER.A) */
	DRIVER( ehrgeiza )	/* Ehrgeiz (EG2/VER.A) */
	DRIVER( mdhorse )	/* Derby Quiz My Dream Horse (MDH1/VER.A2) */
	DRIVER( tenkomor )	/* Tenkomori Shooting (TKM2/VER.A1) */
	DRIVER( tenkomoj )	/* Tenkomori Shooting (TKM1/VER.A1) */
	DRIVER( fgtlayer )	/* Fighting Layer (FTL0/VER.A) */
	DRIVER( pacapp )	/* Paca Paca Passion (PPP1/VER.A2) */
	DRIVER( ptblank2 )	/* Point Blank 2 (GNB5/VER.A) */
	DRIVER( sws99 )		/* Super World Stadium '99 (SS91/VER.A3) */
	DRIVER( tektagt )	/* Tekken Tag Tournament (TEG3/VER.C1) */
	DRIVER( tektagta )	/* Tekken Tag Tournament (TEG3/VER.B) */
	DRIVER( tektagtb )	/* Tekken Tag Tournament (TEG1/VER.B) */
	DRIVER( tektagtc )	/* Tekken Tag Tournament (TEG1/VER.A) */
	DRIVER( ghlpanic )	/* Ghoul Panic (OB2/VER.A) */
	DRIVER( pacapp2 )	/* Paca Paca Passion 2 (PKS1/VER.A) */
	DRIVER( mrdrillr )	/* Mr Driller (DRI1/VER.A2) */
	DRIVER( aquarush )	/* Aqua Rush (AQ1/VER.A1) */
	DRIVER( pacappsp )	/* Paca Paca Passion Special (PSP1/VER.A) */
	DRIVER( golgo13 )	/* Golgo 13 (GLG1/VER.A) */
	DRIVER( g13knd )	/* Golgo 13 Kiseki no Dandou (GLS1/VER.A) */

	/* Namco System 10 */
	DRIVER( mrdrilr2 )	/* Mr Driller 2 (DR21 Ver.A) */
	DRIVER( gjspace )	/* Gekitoride-Jong Space (10011 Ver.A) */
	DRIVER( mrdrilrg )	/* Mr. Driller G (DRG1 Ver.A) */
	DRIVER( knpuzzle )	/* Kotoba no Puzzle Mojipittan (KPM1 Ver.A) */
	DRIVER( startrgn )	/* Star Trigon (STT1 Ver.A) */
	DRIVER( gamshara )	/* Gamshara (10021 Ver.A) */

	/* Konami GQ */
	DRIVER( cryptklr )	/* Crypt Killer (GQ420 UAA) */

	/* Konami GV */
	DRIVER( konamigv )
	DRIVER( kdeadeye )	/* GV054, UA01 */
	DRIVER( pbball96 )	/* GV017, Japan 1.03 */
	DRIVER( hyperath )	/* GV021, Japan 1.00 */
	DRIVER( susume )	/* GV027, Japan 1.20 */
	DRIVER( btchamp )	/* GV053, UAA01 */
	DRIVER( weddingr )	/* GX624, JAA */
	DRIVER( nagano98 )	/* GX720, EAA */
	DRIVER( simpbowl )	/* GQ829, UAA */

	/* Mitchell games */
	DRIVER( mgakuen )	/* (c) 1988 Yuga */
	DRIVER( 7toitsu )	/* (c) 1988 Yuga */
	DRIVER( mgakuen2 )	/* (c) 1989 Face */
	DRIVER( pkladies )	/* (c) 1989 Mitchell */
	DRIVER( pkladiel )	/* (c) 1989 Leprechaun */
	DRIVER( pkladila )	/* (c) 1989 Leprechaun */
	DRIVER( dokaben )	/*  3/1989 (c) 1989 Capcom (Japan) */
				/*  8/1989 Dokaben 2 (baseball) */
	DRIVER( pang )		/* (c) 1989 Mitchell (World) */
	DRIVER( pangb )		/* bootleg */
	DRIVER( bbros )		/* (c) 1989 Capcom (US) not listed on Capcom's site */
	DRIVER( pompingw )	/* (c) 1989 Mitchell (Japan) */
	DRIVER( cbasebal )	/* 10/1989 (c) 1989 Capcom (Japan) (different hardware) */
	DRIVER( cworld )	/* 11/1989 (QUIZ 1) (c) 1989 Capcom */
	DRIVER( hatena )	/* 28/02/1990 (QUIZ 2) (c) 1990 Capcom (Japan) */
	DRIVER( spang )		/* 14/09/1990 (c) 1990 Mitchell (World) */
	DRIVER( spangj )	/* --/--/1990 (c) 1990 Mitchell (Japan) */
	DRIVER( sbbros )	/* 01/10/1990 (c) 1990 Mitchell + Capcom (US) not listed on Capcom's site */
	DRIVER( marukin )	/* 17/10/1990 (c) 1990 Yuga (Japan) */
	DRIVER( qtono1 )	/* 25/12/1990 (QUIZ 3) (c) 1991 Capcom (Japan) */
				/*  4/1991 Ashita Tenki ni Naare (golf) */
	DRIVER( qsangoku )	/* 07/06/1991 (QUIZ 4) (c) 1991 Capcom (Japan) */
	DRIVER( block )		/* 10/09/1991 (c) 1991 Capcom (World) */
	DRIVER( blockj )	/* 10/09/1991 (c) 1991 Capcom (Japan) */
	DRIVER( blockjoy )	/* 06/11/1991 (c) 1991 Capcom (World) (Joystick version, bad dump?) */
	DRIVER( blockbl )	/* bootleg */

	/* Incredible Technologies games */
	/* http://www.itsgames.com */
	DRIVER( capbowl )	/* (c) 1988 Incredible Technologies */
	DRIVER( capbowl2 )	/* (c) 1988 Incredible Technologies */
	DRIVER( capbowl3 )	/* (c) 1988 Incredible Technologies */
	DRIVER( capbowl4 )	/* (c) 1989 Incredible Technologies */
	DRIVER( clbowl )	/* (c) 1989 Incredible Technologies */
	DRIVER( bowlrama )	/* (c) 1991 P & P Marketing */
	DRIVER( wfortune )	/* (c) 1989 GameTek */
	DRIVER( wfortuna )	/* (c) 1989 GameTek */
	DRIVER( stratab )	/* (c) 1990 Strata/Incredible Technologies */
	DRIVER( stratab1 )	/* (c) 1990 Strata/Incredible Technologies */
	DRIVER( sstrike )	/* (c) 1990 Strata/Incredible Technologies */
	DRIVER( gtg )		/* (c) 1990 Strata/Incredible Technologies */
	DRIVER( gtgt )		/* (c) 1990 Strata/Incredible Technologies */
	DRIVER( hstennis )	/* (c) 1990 Strata/Incredible Technologies */
	DRIVER( hstenn10 )	/* (c) 1990 Strata/Incredible Technologies */
	DRIVER( slikshot )	/* (c) 1990 Grand Products/Incredible Technologies */
	DRIVER( sliksh17 )	/* (c) 1990 Grand Products/Incredible Technologies */
	DRIVER( sliksh16 )	/* (c) 1990 Grand Products/Incredible Technologies */
	DRIVER( dynobop )	/* (c) 1990 Grand Products/Incredible Technologies */
	DRIVER( arlingtn )	/* (c) 1991 Strata/Incredible Technologies */
	DRIVER( peggle )	/* (c) 1991 Strata/Incredible Technologies */
	DRIVER( pegglet )	/* (c) 1991 Strata/Incredible Technologies */
	DRIVER( pokrdice )	/* (c) 1991 Strata/Incredible Technologies */
	DRIVER( rimrockn )	/* (c) 1991 Strata/Incredible Technologies */
	DRIVER( rimrck20 )	/* (c) 1991 Strata/Incredible Technologies */
	DRIVER( rimrck16 )	/* (c) 1991 Strata/Incredible Technologies */
	DRIVER( rimrck12 )	/* (c) 1991 Strata/Incredible Technologies */
	DRIVER( ninclown )	/* (c) 1991 Strata/Incredible Technologies */
	DRIVER( gpgolf )	/* (c) 1992 Strata/Incredible Technologies */
	DRIVER( gtg2 )		/* (c) 1992 Strata/Incredible Technologies */
	DRIVER( gtg2t )		/* (c) 1989 Strata/Incredible Technologies */
	DRIVER( gtg2j )		/* (c) 1991 Strata/Incredible Technologies */
	DRIVER( neckneck )	/* (c) 1992 Bundra Games/Incredible Technologies */
	DRIVER( timekill )	/* (c) 1992 Strata/Incredible Technologies */
	DRIVER( timek131 )	/* (c) 1992 Strata/Incredible Technologies */
	DRIVER( hardyard )	/* (c) 1993 Strata/Incredible Technologies */
	DRIVER( hardyd10 )	/* (c) 1993 Strata/Incredible Technologies */
	DRIVER( bloodstm )	/* (c) 1994 Strata/Incredible Technologies */
	DRIVER( bloods22 )	/* (c) 1994 Strata/Incredible Technologies */
	DRIVER( bloods21 )	/* (c) 1994 Strata/Incredible Technologies */
	DRIVER( bloods11 )	/* (c) 1994 Strata/Incredible Technologies */
	DRIVER( pairs )		/* (c) 1994 Strata/Incredible Technologies */
	DRIVER( pairsa )	/* (c) 1994 Strata/Incredible Technologies */
	DRIVER( hotmemry )	/* (c) 1994 Tuning/Incredible Technologies */
	DRIVER( drivedge )	/* (c) 1994 Strata/Incredible Technologies */
	DRIVER( wcbowl )	/* (c) 1995 Incredible Technologies */
	DRIVER( wcbwl165 )	/* (c) 1995 Incredible Technologies */
	DRIVER( wcbwl161 )	/* (c) 1995 Incredible Technologies */
	DRIVER( wcbwl140 )	/* (c) 1995 Incredible Technologies */
	DRIVER( wcbwl12 )	/* (c) 1995 Incredible Technologies */
	DRIVER( sftm )		/* (c) 1995 Capcom/Incredible Technologies */
	DRIVER( sftm110 )	/* (c) 1995 Capcom/Incredible Technologies */
	DRIVER( sftm111 )	/* (c) 1995 Capcom/Incredible Technologies */
	DRIVER( sftmj )		/* (c) 1995 Capcom/Incredible Technologies */
	DRIVER( shufshot )	/* (c) Strata/Incredible Technologies */
	DRIVER( sshot139 )	/* (c) Strata/Incredible Technologies */
	DRIVER( sshot137 )	/* (c) Strata/Incredible Technologies */
	DRIVER( wcbowldx )	/* (c) 1999 Incredible Technologies */

	DRIVER( gt3d )		/* (c) 1995 Incredible Technologies */
	DRIVER( gt3dl192 )	/* (c) 1995 Incredible Technologies */
	DRIVER( gt3dl191 )	/* (c) 1995 Incredible Technologies */
	DRIVER( gt3ds192 )	/* (c) 1995 Incredible Technologies */
	DRIVER( gt3dv18 )	/* (c) 1995 Incredible Technologies */
	DRIVER( gt3dv17 )	/* (c) 1995 Incredible Technologies */
	DRIVER( gt3dv16 )	/* (c) 1995 Incredible Technologies */
	DRIVER( gt3dv15 )	/* (c) 1995 Incredible Technologies */
	DRIVER( gt3dv14 )	/* (c) 1995 Incredible Technologies */
	DRIVER( gt3dt231 )	/* (c) 1995 Incredible Technologies */
	DRIVER( gt3dt211 )	/* (c) 1995 Incredible Technologies */
	DRIVER( gt97 )		/* (c) 1997 Incredible Technologies */
	DRIVER( gt97v122 )	/* (c) 1997 Incredible Technologies */
	DRIVER( gt97v121 )	/* (c) 1997 Incredible Technologies */
	DRIVER( gt97s121 )	/* (c) 1997 Incredible Technologies */
	DRIVER( gt97v120 )	/* (c) 1997 Incredible Technologies */
	DRIVER( gt97t240 )	/* (c) 1997 Incredible Technologies */
	DRIVER( gt98 )		/* (c) 1998 Incredible Technologies */
	DRIVER( gt98v100 )	/* (c) 1998 Incredible Technologies */
	DRIVER( gt98s100 )	/* (c) 1998 Incredible Technologies */
	DRIVER( gt98t303 )	/* (c) 1998 Incredible Technologies */
	DRIVER( gt99 )		/* (c) 1999 Incredible Technologies */
	DRIVER( gt99s100 )	/* (c) 1999 Incredible Technologies */
	DRIVER( gt99t400 )	/* (c) 1999 Incredible Technologies */
	DRIVER( gtroyal )	/* (c) 1999 Incredible Technologies */
	DRIVER( gt2k )		/* (c) 2000 Incredible Technologies */
	DRIVER( gt2ks100 )	/* (c) 2000 Incredible Technologies */
	DRIVER( gt2kt500 )	/* (c) 2000 Incredible Technologies */
	DRIVER( gtsuprem )	/* (c) 2000 Incredible Technologies */
	DRIVER( gtclassc )	/* (c) 2001 Incredible Technologies */
	DRIVER( gtclassp )	/* (c) 2001 Incredible Technologies */
	DRIVER( gtcls100 )	/* (c) 2001 Incredible Technologies */

	/* Leland games */
	DRIVER( cerberus )	/* (c) 1985 Cinematronics */
	DRIVER( mayhem )	/* (c) 1985 Cinematronics */
	DRIVER( powrplay )	/* (c) 1985 Cinematronics */
	DRIVER( wseries )	/* (c) 1985 Cinematronics */
	DRIVER( alleymas )	/* (c) 1986 Cinematronics */
	DRIVER( upyoural )	/* (c) 1987 Cinematronics */
	DRIVER( dangerz )	/* (c) 1986 Cinematronics USA */
	DRIVER( basebal2 )	/* (c) 1987 Cinematronics */
	DRIVER( dblplay )	/* (c) 1987 Tradewest / Leland */
	DRIVER( strkzone )	/* (c) 1988 Leland */
	DRIVER( redlin2p )	/* (c) 1987 Cinematronics + Tradewest license */
	DRIVER( quarterb )	/* (c) 1987 Leland */
	DRIVER( quartrba )	/* (c) 1987 Leland */
	DRIVER( viper )		/* (c) 1988 Leland */
	DRIVER( teamqb )	/* (c) 1988 Leland */
	DRIVER( teamqb2 )	/* (c) 1988 Leland */
	DRIVER( aafb )		/* (c) 1989 Leland */
	DRIVER( aafbd2p )	/* (c) 1989 Leland */
	DRIVER( aafbc )		/* (c) 1989 Leland */
	DRIVER( aafbb )		/* (c) 1989 Leland */
	DRIVER( offroad )	/* (c) 1989 Leland */
	DRIVER( offroadt )	/* (c) 1989 Leland */
	DRIVER( pigout )	/* (c) 1990 Leland */
	DRIVER( pigouta )	/* (c) 1990 Leland */
	DRIVER( ataxx )		/* (c) 1990 Leland */
	DRIVER( ataxxa )	/* (c) 1990 Leland */
	DRIVER( ataxxj )	/* (c) 1990 Leland */
	DRIVER( wsf )		/* (c) 1990 Leland */
	DRIVER( indyheat )	/* (c) 1991 Leland */
	DRIVER( brutforc )	/* (c) 1991 Leland */
	DRIVER( asylum )	/* (c) 1991 Leland */

	/* Gremlin 8080 games */
	/* the numbers listed are the range of ROM part numbers */
	DRIVER( blockade )	/* 1-4 [1977 Gremlin] */
	DRIVER( comotion )	/* 5-7 [1977 Gremlin] */
	DRIVER( hustle )	/* 16-21 [1977 Gremlin] */
	DRIVER( blasto )	/* [1978 Gremlin] */
	DRIVER( mineswpr )	/* [1977 Amutech] */
	DRIVER( minesw4p )	/* [1977 Amutech] */

	/* Gremlin/Sega "VIC dual game board" games */
	/* the numbers listed are the range of ROM part numbers */
	DRIVER( depthch )	/* 50-55 [1977 Gremlin?] */
	DRIVER( depthv1 )	/*   ?   [1977 Gremlin?] */
	DRIVER( subhunt )	/*   ?   [1977 Taito] */
	DRIVER( safari )	/* 57-66 [1977 Gremlin?] */
	DRIVER( frogs )		/* 112-119 [1978 Gremlin?] */
	DRIVER( sspaceat )	/* 155-162 (c) */
	DRIVER( sspacat2 )
	DRIVER( sspacat3 )
	DRIVER( sspacatc )	/* 139-146 (c) */
	DRIVER( sspacaho )	/* ? epr00001.bin - epr00008.bin */
	DRIVER( headon )	/* 163-167/192-193 (c) Gremlin */
	DRIVER( headonb )	/* 163-167/192-193 (c) Gremlin */
	DRIVER( headon2 )	/* ???-??? (c) 1979 Sega */
				/* ???-??? Fortress */
				/* ???-??? Gee Bee */
				/* 255-270  Head On 2 / Deep Scan */
	DRIVER( invho2 )	/* 271-286 (c) 1979 Sega */
	DRIVER( nsub )		/* 268-275 (c) 1980 Sega */
	DRIVER( samurai )	/* 289-302 + upgrades (c) 1980 Sega */
	DRIVER( invinco )	/* 310-318 (c) 1979 Sega */
	DRIVER( invds )		/* 367-382 (c) 1979 Sega */
	DRIVER( tranqgun )	/* 413-428 (c) 1980 Sega */
				/* 450-465  Tranquilizer Gun (different version?) */
				/* ???-??? Car Hunt / Deep Scan */
	DRIVER( spacetrk )	/* 630-645 (c) 1980 Sega */
	DRIVER( sptrekct )	/* (c) 1980 Sega */
	DRIVER( carnival )	/* 651-666 (c) 1980 Sega */
	DRIVER( carnvckt )	/* 501-516 (c) 1980 Sega */
	DRIVER( brdrline )	/* (c) 1980 Sega */
	DRIVER( brdrlinb )	/* bootleg */
	DRIVER( digger )	/* 684-691 no copyright notice */
	DRIVER( pulsar )	/* 790-805 (c) 1981 Sega */
	DRIVER( heiankyo )	/* (c) [1979?] Denki Onkyo */
	DRIVER( alphaho )	/* Data East */

	/* Sega G-80 vector games */
	DRIVER( spacfury )	/* (c) 1981 */
	DRIVER( spacfura )	/* no copyright notice */
	DRIVER( zektor )	/* (c) 1982 */
	DRIVER( tacscan )	/* (c) 1982 */
	DRIVER( elim2 )		/* (c) 1981 Gremlin */
	DRIVER( elim2a )	/* (c) 1981 Gremlin */
	DRIVER( elim2c )	/* (c) 1981 Gremlin */
	DRIVER( elim4 )		/* (c) 1981 Gremlin */
	DRIVER( elim4p )	/* (c) 1981 Gremlin */
	DRIVER( startrek )	/* (c) 1982 */

	/* Sega G-80 raster games */
	DRIVER( astrob )	/* (c) 1981 */
	DRIVER( astrob2 )	/* (c) 1981 */
	DRIVER( astrob2a )	/* (c) 1981 */
	DRIVER( astrob1 )	/* (c) 1981 */
	DRIVER( 005 )		/* (c) 1981 */
	DRIVER( monsterb )	/* (c) 1982 */
	DRIVER( monster2 )	/* (c) 1982 */
	DRIVER( spaceod )	/* (c) 1981 */
	DRIVER( pignewt )	/* (c) 1983 */
	DRIVER( pignewta )	/* (c) 1983 */
	DRIVER( sindbadm )	/* 834-5244 (c) 1983 Sega */

	/* Sega "Zaxxon hardware" games */
	DRIVER( zaxxon )	/* (c) 1982 */
	DRIVER( zaxxon2 )	/* (c) 1982 */
	DRIVER( zaxxonb )	/* bootleg */
	DRIVER( szaxxon )	/* (c) 1982 */
	DRIVER( futspy )	/* (c) 1984 */
	DRIVER( razmataz )	/* modified 834-0213, 834-0214 (c) 1983 */
	DRIVER( ixion )		/* (c) 1983 */
	DRIVER( congo )		/* 605-5167 (c) 1983 */
	DRIVER( tiptop )	/* 605-5167 (c) 1983 */

	/* Sega SG1000 based games */
	DRIVER( chboxing )	/* (c) 1984 */
	DRIVER( chwrestl )	/* (c) 1985 */

	/* Sega System 1 / System 2 games */
	DRIVER( starjack )	/* 834-5191 (c) 1983 (S1) */
	DRIVER( starjacs )	/* (c) 1983 Stern (S1) */
	DRIVER( regulus )	/* 834-5328?(c) 1983 (S1) */
	DRIVER( reguluso )	/* 834-5328?(c) 1983 (S1) */
	DRIVER( regulusu )	/* 834-5328?(c) 1983 (S1) */
	DRIVER( upndown )	/* (c) 1983 (S1) */
	DRIVER( upndownu )	/* (c) 1983 (S1) */
	DRIVER( mrviking )	/* 834-5383 (c) 1984 (S1) */
	DRIVER( mrvikngj )	/* 834-5383 (c) 1984 (S1) */
	DRIVER( swat )		/* 834-5388 (c) 1984 Coreland / Sega (S1) */
	DRIVER( flicky )	/* (c) 1984 (S1) */
	DRIVER( flickyo )	/* (c) 1984 (S1) */
	DRIVER( wmatch )	/* (c) 1984 (S1) */
	DRIVER( bullfgt )	/* 834-5478 (c) 1984 Sega / Coreland (S1) */
	DRIVER( thetogyu )	/* 834-5478 (c) 1984 Sega / Coreland (S1) */
	DRIVER( spatter )	/* 834-5583 (c) 1984 (S1) */
	DRIVER( ssanchan )	/* 834-5583 (c) 1984 (S1) */
	DRIVER( pitfall2 )	/* 834-5627 [1985?] reprogrammed, (c) 1984 Activision (S1) */
	DRIVER( pitfallu )	/* 834-5627 [1985?] reprogrammed, (c) 1984 Activision (S1) */
	DRIVER( seganinj )	/* 834-5677 (c) 1985 (S1) */
	DRIVER( seganinu )	/* 834-5677 (c) 1985 (S1) */
	DRIVER( ninja )		/* 834-5677 (c) 1985 (S1) */
	DRIVER( nprinces )	/* 834-5677 (c) 1985 (S1) */
	DRIVER( nprincso )	/* 834-5677 (c) 1985 (S1) */
	DRIVER( nprincsu )	/* 834-5677 (c) 1985 (S1) */
	DRIVER( nprincsb )	/* bootleg? (S1) */
	DRIVER( imsorry )	/* 834-5707 (c) 1985 Coreland / Sega (S1) */
	DRIVER( imsorryj )	/* 834-5707 (c) 1985 Coreland / Sega (S1) */
	DRIVER( teddybb )	/* 834-5712 (c) 1985 (S1) */
	DRIVER( teddybbo )	/* 834-5712 (c) 1985 (S1) */
	DRIVER( hvymetal )	/* 834-5745 (c) 1985 (S2?) */
	DRIVER( myhero )	/* 834-5755 (c) 1985 (S1) */
	DRIVER( sscandal )	/* 834-5755 (c) 1985 Coreland / Sega (S1) */
	DRIVER( myherok )	/* 834-5755 (c) 1985 Coreland / Sega (S1) */
	DRIVER( shtngmst )	/* 834-5719/5720 (c) 1985 (S2) */
	DRIVER( chplft )	/* 834-5795 (c) 1985, (c) 1982 Dan Gorlin (S2) */
	DRIVER( chplftb )	/* 834-5795 (c) 1985, (c) 1982 Dan Gorlin (S2) */
	DRIVER( chplftbl )	/* bootleg (S2) */
	DRIVER( 4dwarrio )	/* 834-5918 (c) 1985 Coreland / Sega (S1) */
	DRIVER( brain )		/* (c) 1986 Coreland / Sega (S2?) */
	DRIVER( raflesia )	/* 834-5753 (c) 1985 Coreland / Sega (S1) */
	DRIVER( wboy )		/* 834-5984 (c) 1986 + Escape license (S1) */
	DRIVER( wboyo )		/* 834-5984 (c) 1986 + Escape license (S1) */
	DRIVER( wboy2 )		/* 834-5984 (c) 1986 + Escape license (S1) */
	DRIVER( wboy2u )	/* 834-5984 (c) 1986 + Escape license (S1) */
	DRIVER( wboy3 )		/* 834-5984 (c) 1986 + Escape license (S1) */
	DRIVER( wboyu )		/* 834-5753 (? maybe a conversion) (c) 1986 + Escape license (S1) */
	DRIVER( wbdeluxe )	/* (c) 1986 + Escape license (S1) */
	DRIVER( wboysys2 )      /* 834-5984  (c) 1986 + Escape license (S1) */
	DRIVER( gardia )	/* 834-6119 (S2?) */
	DRIVER( gardiab )	/* bootleg */
	DRIVER( noboranb )	/* bootleg */
	DRIVER( blockgal )	/* 834-6303 (S1) */
	DRIVER( blckgalb )	/* bootleg */
	DRIVER( tokisens )	/* (c) 1987 (from a bootleg board) (S2) */
	DRIVER( wbml )		/* bootleg (S2) */
	DRIVER( wbmljo )	/* (c) 1987 Sega/Westone (S2) */
	DRIVER( wbmljb )	/* (c) 1987 Sega/Westone (S2) */
	DRIVER( wbmlb )		/* bootleg? (S2) */
	DRIVER( dakkochn )	/* 836-6483? (S2) */
	DRIVER( ufosensi )	/* 834-6659 (S2) */
	DRIVER( ufosensb )	/* bootleg */
/*
other System 1 / System 2 games:

WarBall
Sanrin Sanchan
DokiDoki Penguin Land *not confirmed
*/

	/* Sega System E games (Master System hardware) */
	DRIVER( hangonjr )	/* (c) 1985 */
	DRIVER( transfrm )	/* 834-5803 (c) 1986 */
	DRIVER( astrofl )	/* 834-5803 (c) 1986 */
	DRIVER( ridleofp )	/* (c) 1986 Sega / Nasco */
	DRIVER( fantzn2 )
	DRIVER( opaopa )
	DRIVER( tetrisse )	/* (c) 1988 */

	/* other Sega 8-bit games */
	DRIVER( turbo )		/* (c) 1981 Sega */
	DRIVER( turboa )	/* (c) 1981 Sega */
	DRIVER( turbob )	/* (c) 1981 Sega */
	DRIVER( subroc3d )	/* (c) 1982 Sega */
	DRIVER( buckrog )	/* (c) 1982 Sega */
	DRIVER( buckrogn )	/* (c) 1982 Sega */
	DRIVER( zoom909 )	/* (c) 1982 Sega */
	DRIVER( kopunch )	/* 834-0103 (c) 1981 Sega */
	DRIVER( suprloco )	/* (c) 1982 Sega */
	DRIVER( dotrikun )	/* cabinet test board */
	DRIVER( dotriku2 )	/* cabinet test board */
	DRIVER( spcpostn )	/* (c) 1986 Sega / Nasco" */
	DRIVER( angelkds )	/* 833-6599 (c) 1988 Sega / Nasco? */
	DRIVER( calorie )	/* (c) 1986 Sega */
	DRIVER( calorieb )	/* (c) 1986 Sega (but bootleg) */

	/* Enduro Racer / Space Harrier / Hang On */
	DRIVER( enduror )	/* (c) 1985 (FD1089B, decrypted) */
	DRIVER( enduror1 )	/* (c) 1985 (FD1089B, decrypted) */
	DRIVER( endurob2 )	/* (c) 1985 (Beta bootleg) */
	DRIVER( endurobl )	/* (c) 1985 (Herb bootleg) */
	DRIVER( hangon )	/* (c) 1985 */
	DRIVER( sharrier )	/* (c) 1985 */
	DRIVER( sharrirb )	/* (c) 1985 */
	DRIVER( shangupb )	/* (c) 1992 (but bootleg, hangon hw?) */

	/* Pre-16 */
	DRIVER( aliensy1 )	/* (c) 1987 (FD1094, decrypted) */
	DRIVER( aliensy2 )	/* (c) 1987 (FD1094, decrypted) */
	DRIVER( bodyslam )	/* (c) 1986 (8751) */
	DRIVER( dumpmtmt )	/* (c) 1986 (Japan) */
	DRIVER( mjleague )	/* (c) 1985 */
	DRIVER( quartet )	/* (c) 1986 (8751) */
	DRIVER( quartetj )	/* (c) 1986 (8751) */
	DRIVER( quartet2 )	/* (c) 1986 (8751) */
	DRIVER( quartt2j )	/* (c) 1986 (8751) */

	/* System16A */
	DRIVER( afighter )	/* (c) 1986 (FD1089A, decrypted) */
	DRIVER( alexkidd )	/* (c) 1986 (FD1089A, decrypted) */
	DRIVER( alexkid1 )	/* (c) 1986 */
	DRIVER( fantzone )	/* (c) 1986 */
	DRIVER( fantzon1 )	/* (c) 1986 */
	DRIVER( sdi )		/* (c) 1987 (FD1089B, decrypted) */
	DRIVER( shinobi )	/* (c) 1987 */
	DRIVER( shinobi1 )	/* (c) 1987 (FD1094, decrypted) */
	DRIVER( sjryuko1 )	/* (c) 1988 White Board (FD1089B, decrypted) */
	DRIVER( tetris )	/* (c) 1988 (FD1094, decrypted) S16A */
	DRIVER( tetris3 )	/* (c) 1988 (FD1094, decrypted) S16A */
	DRIVER( timesca1 )	/* (c) 1987 (FD1089B, decrypted) */
	DRIVER( wb31 )		/* (c) 1988 Sega / Westone (FD1094, decrypted) */

	/* System 16B */
	DRIVER( aceattac )	/* (c) 1988 (protected) */
	DRIVER( aliensyn )	/* (c) 1987 */
	DRIVER( aliensy3 )	/* (c) 1987 (FD1089A, decrypted) */
	DRIVER( altbeast )	/* (c) 1988 (8751) */
	DRIVER( altbeasj )	/* (c) 1988 (8751) */
	DRIVER( altbeas5 )	/* (c) 1988 (8751) */
	DRIVER( altbeas4 )	/* (c) 1988 (MC-8123B) */
	DRIVER( altbeaj3 )	/* (c) 1988 (FD1094, decrypted) */
	DRIVER( altbeas2 )	/* (c) 1988 (MC-8123B) */
	DRIVER( altbeaj1 )	/* (c) 1988 (protected) */
	DRIVER( aurail )	/* (c) 1990 Sega / Westone */
	DRIVER( aurail1 )	/* (c) 1990 Sega / Westone (FD1089B, decrypted) */
	DRIVER( aurailj )	/* (C) 1990 Sega / Westone (FD1089A, decrypted) */
	DRIVER( bayroute )	/* (c) 1989 Sunsoft / Sega (FD1094, decrypted) */
	DRIVER( bayroutj )	/* (c) 1989 Sunsoft / Sega (FD1094, decrypted) */
	DRIVER( bayrout1 )	/* (c) 1989 Sunsoft / Sega */
	DRIVER( bullet )	/* (c) 1987 (protected) */
	DRIVER( cotton )	/* (c) 1990 (FD1094, decrypted) */
	DRIVER( cottonu )	/* (c) 1990 (FD1094, decrypted) */
	DRIVER( cottonj )	/* (c) 1990 (FD1094, decrypted) */
	DRIVER( ddux )		/* (c) 1989 (FD1094, decrypted) */
	DRIVER( ddux1 )		/* (c) 1989 (8751) */
	DRIVER( dunkshot )	/* (c) 1986 (FD1094, decrypted) */
	DRIVER( eswat )		/* (c) 1989 (FD1094, decrypted) */
	DRIVER( eswatu )	/* (c) 1989 (FD1094, decrypted) */
	DRIVER( eswatj )	/* (c) 1989 (FD1094, decrypted) */
	DRIVER( exctleag )	/* (protected) */
	DRIVER( fpoint )	/* (c) 1989 (Japan, FD1094, decrypted) */
	DRIVER( fpoint1 )	/* (c) 1989 (Japan, FD1094, decrypted) */
	DRIVER( goldnaxe )	/* (c) 1989 (8751) */
	DRIVER( goldnaxu )	/* (c) 1989 (protected) */
	DRIVER( goldnaxj )	/* (c) 1989 (FD1094, decrypted) */
	DRIVER( goldnax3 )	/* (c) 1989 (FD1094, decrypted) */
	DRIVER( goldnax2 )	/* (c) 1989 (8751) */
	DRIVER( goldnax1 )	/* (c) 1989 (FD1094, decrypted) */
	DRIVER( hwchamp )	/* (c) 1987 */
	DRIVER( mvp )		/* (c) 1989 (FD1094, decrypted) */
	DRIVER( mvpj )		/* (c) 1989 (FD1094, decrypted) */
	DRIVER( passsht )	/* (c) 1988 (FD1094, decrypted) */
	DRIVER( passshta )	/* (c) 1988 (FD1094, decrypted) */
	DRIVER( passshtj )	/* (c) 1988 (FD1094, decrypted) */
	DRIVER( riotcity )	/* (c) 1991 Sega / Westone */
	DRIVER( ryukyu )	/* (c) 1990 (FD1094, decrypted) */
	DRIVER( sdib )		/* (c) 1987 (FD1089A, decrypted) */
	DRIVER( sdibl )		/* (c) 1987 bootleg */
	DRIVER( defense )	/* (c) 1987 (FD1094, decrypted) */
	DRIVER( shinobi4 )	/* (c) 1987 */
	DRIVER( shinobi3 )	/* (c) 1987 */
	DRIVER( shinobi2 )	/* (c) 1987 (FD1094, decrypted) */
	DRIVER( sonicbom )	/* (c) 1987 (FD1094, decrypted) */
	DRIVER( sjryuko )	/* (c) 1988 White Board (FD1094, decrypted) */
	DRIVER( suprleag )	/* (c) 1987 (protected) */
	DRIVER( tetris2 )	/* (c) 1988 (FD1094, decrypted) S16B */
	DRIVER( tetris1 )	/* (c) 1988 (FD1094, decrypted) S16B */
	DRIVER( timescan )	/* (c) 1987 */
	DRIVER( toryumon )	/* (c) 1995 */
	DRIVER( tturf )		/* (c) 1989 Sega / Sunsoft (8751) */
	DRIVER( tturfu )	/* (c) 1989 Sega / Sunsoft (8751) */
	DRIVER( wb3 )		/* (c) 1988 Sega / Westone (8751) */
	DRIVER( wb34 )		/* (c) 1988 Sega / Westone (FD1094, decrypted) */
	DRIVER( wb33 )		/* (c) 1988 Sega / Westone (FD1094, decrypted) */
	DRIVER( wb32 )		/* (c) 1988 Sega / Westone (FD1094, decrypted) */
	DRIVER( wrestwar )	/* (c) 1989 (8751) */
	DRIVER( wrestwa2 )	/* (c) 1989 (FD1094, decrypted) */
	DRIVER( wrestwa1 )	/* (c) 1989 (FD1094, decrypted) */

	/* Philko Boards */
	DRIVER( atomicp )	/* (c) 1990 Philko - korean clone board */
	DRIVER( snapper )	/* (c) 1990 Philko - korean clone board */


	/* S16 Bootlegs */
	DRIVER( bayrtbl1 )	/* (c) 1989 (protected) (bootleg) */
	DRIVER( bayrtbl2 )	/* (c) 1989 (protected) (bootleg) */
	DRIVER( dduxbl )	/* (c) 1989 (Datsu bootleg) */
	DRIVER( eswatbl )	/* BOOTLEG */
	DRIVER( fpointbl )	/* (c) 1989 (Datsu bootleg) */
	DRIVER( fpointbj )	/* (c) 1989 (Datsu bootleg, Japan) */
	DRIVER( goldnabl )	/* BOOTLEG  */
	DRIVER( passht4b )	/* bootleg */
	DRIVER( passshtb )	/* bootleg */
	DRIVER( shinobl )	/* (c) 1987 (but bootleg) */
	DRIVER( tetrisbl )	/* (c) 1988 (but bootleg) */
	DRIVER( tturfbl )	/* (c) 1989 (Datsu bootleg) */
	DRIVER( wb3bbl )	/* BOOTLEG */

	/* System 18 */
	DRIVER( astorm )	/* (c) 1990 (protected 2 Players) */
	DRIVER( astorm3 )	/* (c) 1990 (FD1094, decrypted) */
	DRIVER( astormu )	/* (c) 1990 (FD1094, decrypted) */
	DRIVER( astormj )	/* (c) 1990 (FD1094, decrypted) */
	DRIVER( bloxeed )	/* (c) 1990 (FD1094, decrypted) */
	DRIVER( shdancer )	/* (c) 1989 */
	DRIVER( shdancej )	/* (c) 1989 */
	DRIVER( shdance1 )	/* (c) 1989 */
	DRIVER( ddcrew )	/* (c) 1991 (FD1094, decrypted) */
	DRIVER( ddcrewu )	/* (c) 1991 (FD1094, decrypted) */
	DRIVER( ddcrew2 )	/* (c) 1991 (FD1094, decrypted) */
	DRIVER( ddcrew1 )	/* (c) 1991 (FD1094, decrypted) */
	DRIVER( ddcrewj )	/* (c) 1991 (FD1094, decrypted) */
	DRIVER( cltchitr )	/* (c) 1991 (FD1094, decrypted) */
	DRIVER( cltchtrj )	/* (c) 1991 (FD1094, decrypted) */
	DRIVER( desertbr )	/* (c) 1993 (FD1094, decrypted) */
	DRIVER( mwalk )		/* (c) 1990 (FD1094+8751, decrypted) */
	DRIVER( mwalku )	/* (c) 1990 (FD1094+8751, decrypted) */
	DRIVER( mwalkj )	/* (c) 1990 (FD1094+8751, decrypted) */
	DRIVER( pontoon )
	DRIVER( lghost )	/* (c) 1990 (FD1094, decrypted) */
	DRIVER( lghostu )	/* (c) 1990 (FD1094, decrypted) */
	DRIVER( wwallyj )	/* (c) 1992 (FD1094, decrypted) */
	DRIVER( wwallyja )	/* (c) 1992 (FD1094, decrypted) */

	/* System18 Bootlegs */
	DRIVER( astormbl )	/* BOOTLEG */
	DRIVER( shdancbl )	/* BOOTLEG */
	DRIVER( mwalkbl )	/* BOOTLEG */


	/* Outrun Hardware */
	DRIVER( outrun )	/* (c) 1986 (bootleg)*/
	DRIVER( outrun2 )	/* (c) 1986 (bootleg) */
	DRIVER( outrun1 )	/* (c) 1986 Deluxe? */
	DRIVER( outrunb )	/* (c) 1986 (protected beta bootleg) */
	DRIVER( toutrun )	/* (c) 1987 (FD1094, decrypted) */
	DRIVER( toutrun2 )	/* (c) 1987 (FD1094, decrypted) */
	DRIVER( toutrun1 )	/* (c) 1987 (protected) */
	DRIVER( shangon )	/* (c) 1987 (protected) */
	DRIVER( shangon3 )	/* (c) 1987 (FD1089B, decrypted) */
	DRIVER( shangon2 )	/* (c) 1987 (FD1089B, decrypted) */
	DRIVER( shangon1 )	/* (c) 1987 (FD1089B, decrypted) */
	DRIVER( shangnle )	/* (c) 1991 */

	/* X Board */
	DRIVER( aburner )	/* (c) 1987 */
	DRIVER( aburner2 )	/* (c) 1987 */
	DRIVER( thndrbld )	/* (c) 1987 (FD1094, decrypted) */
	DRIVER( thndrbdj )	/* (c) 1987 */
	DRIVER( loffire )	/* (c) 1989 (FD1094, decrypted) */
	DRIVER( loffireu )	/* (c) 1989 (FD1094, decrypted) */
	DRIVER( loffirej )	/* (c) 1989 (FD1094, decrypted) */
	DRIVER( rachero )	/* (c) 1989 (FD1094, decrypted) */
	DRIVER( abcop )		/* (c) 1990 (FD1094, decrypted) */

	DRIVER( gprider )	/* (c) 1990 (protected) */
	DRIVER( gprider1 )	/* (c) 1990 (FD1094, decrypted) */
	DRIVER( smgp )		/* (c) 1989 (FD1094, decrypted) */
	DRIVER( smgp6 )		/* (c) 1989 (FD1094, decrypted) */
	DRIVER( smgp5 )		/* (c) 1989 (FD1094, decrypted) */
	DRIVER( smgpu )		/* (c) 1989 (FD1094, decrypted) */
	DRIVER( smgpu3 )	/* (c) 1989 (FD1094, decrypted) */
	DRIVER( smgpu2 )	/* (c) 1989 (FD1094, decrypted) */
	DRIVER( smgpu1 )	/* (c) 1989 (FD1094, decrypted) */
	DRIVER( smgpj )		/* (c) 1989 (FD1094, decrypted) */

	/* Y-Board */
	DRIVER( gforce2 )	/* (c) 1988 */
	DRIVER( gforce2j )	/* (c) 1988 */
	DRIVER( gloc )		/* (c) 1990 */
	DRIVER( glocr360 )	/* (c) 1990 */
	DRIVER( strkfgtr )	/* (c) 1991 */
	DRIVER( pdrift )	/* (c) 1988 */
	DRIVER( pdrifta )	/* (c) 1988 */
	DRIVER( pdriftj )	/* (c) 1988 */
	DRIVER( rchase )	/* (c) 1991 */

/*
Sega System 24 game list
Apr.1988 Hot Rod
Oct.1988 Scramble Spirits
Nov.1988 Gain Ground
Apr.1989 Crack Down
Aug.1989 Jumbo Ozaki Super Masters
Jun.1990 Bonanza Bros.
Dec.1990 Rough Racer
Feb.1991 Quiz Syukudai wo Wasuremashita
Jul.1991 Dynamic C.C.
Dec.1991 Quiz Rouka ni Tattenasai
Dec.1992 Tokorosan no MahMahjan
May.1993 Quiz Mekurumeku Story
May.1994 Tokorosan no MahMahjan 2
Sep.1994 Quiz Ghost Hunter
*/
	/* disk based */
	DRIVER( hotrod )	/* (c) 1988 */
	DRIVER( hotroda )	/* (c) 1988 */
	DRIVER( hotrodj )	/* (c) 1988 */
	DRIVER( sspirits ) 	/* (c) 1988 */
	DRIVER( sspiritj ) 	/* (c) 1988 */
	DRIVER( sspirtfc )	/* (c) 1988 (protected) */
	DRIVER( gground )	/* (c) 1988 (FD1094, decrypted) */
	DRIVER( crkdown )	/* (c) 1989 (FD1094, decrypted) */
	DRIVER( crkdownj )	/* (c) 1989 (FD1094, decrypted) */
	DRIVER( sgmast )	/* (c) 1989 (protected) */
	DRIVER( sgmastc )	/* (c) 1989 (FD1094, decrypted) */
	DRIVER( sgmastj )	/* (c) 1989 (FD1094, decrypted) */
	DRIVER( roughrac )	/* (c) 1990 (FD1094, decrypted) */
	DRIVER( bnzabros )	/* (c) 1990 */
	DRIVER( bnzabrsj )	/* (c) 1990 */
	DRIVER( qsww )		/* (c) 1991 */
	DRIVER( dcclubfd )	/* (c) 1991 */

	/* ROM based */
	DRIVER( dcclub )	/* (c) 1991 */
	DRIVER( mahmajn )	/* (c) 1992 */
	DRIVER( qgh )		/* (c) 1994 */
	DRIVER( quizmeku )	/* (c) 1994 */
	DRIVER( qrouka )	/* (c) 1994 */
	DRIVER( mahmajn2 )	/* (c) 1994 */

	/* Sega System 32 games */
	DRIVER( holo )		/* (c) 1992 (US) */
	DRIVER( svf )		/* (c) 1994 */
	DRIVER( svs )		/* (c) 1994 */
	DRIVER( jleague )	/* (c) 1994 (Japan) */
	DRIVER( brival )	/* (c) 1992 (World) */
	DRIVER( brivalj )	/* (c) 1992 (Japan) */
	DRIVER( radm )		/* (c) 1991 (World) */
	DRIVER( radmu )		/* (c) 1991 (US) */
	DRIVER( radr )		/* (c) 1991 (World) */
	DRIVER( radru )		/* (c) 1991 (US) */
	DRIVER( f1en )		/* (c) 1991 */
	DRIVER( alien3 )	/* (c) 1993 */
	DRIVER( alien3u )	/* (c) 1993 */
	DRIVER( sonic )		/* (c) 1992 (Japan) */
	DRIVER( sonicp )	/* (c) 1992 (Japan) */
	DRIVER( jpark )		/* (c) 1994 */
	DRIVER( ga2 )		/* (c) 1992 (World) */
	DRIVER( ga2u )		/* (c) 1992 (US) */
	DRIVER( ga2j )		/* (c) 1992 (Japan) */
	DRIVER( spidman )	/* (c) 1991 (World) */
	DRIVER( spidmanu )	/* (c) 1991 (US) */
	DRIVER( arabfgt )	/* (c) 1991 */
	DRIVER( arabfgtu )	/* (c) 1991 (US) */
	DRIVER( arabfgtj )	/* (c) 1991 (Japan) */
	DRIVER( arescue )	/* (c) 1992 */
	DRIVER( f1lap )		/* (c) 1993 */
	DRIVER( dbzvrvs )	/* (c) 1994 Sega/Banpresto */
	DRIVER( darkedge )	/* (c) 1992 */
	DRIVER( darkedgj )	/* (c) 1992 (Japan) */
	DRIVER( slipstrm )	/* (c) 1995 Capcom */

	/* Sega Multi System 32 games */
	DRIVER( orunners )	/* (c) 1992 (World) */
	DRIVER( orunneru )	/* (c) 1992 (US) */
	DRIVER( harddunk )	/* (c) 1994 (World) */
	DRIVER( harddunj )	/* (c) 1994 (Japan) */
	DRIVER( titlef )	/* (c) 1992 (World) */
	DRIVER( titlefu )	/* (c) 1992 (US) */
	DRIVER( scross )	/* (c) 1992 (World) */

	/* Model 1 Games */
	DRIVER( vf )		/* (c) 1993 */
	DRIVER( vr )		/* (c) 1992 */
	DRIVER( vformula )	/* (c) 1993 */
	DRIVER( swa )		/* (c) 1993 */
	DRIVER( wingwar )	/* (c) 1994 */
	DRIVER( wingwara )	/* (c) 1994 (US) */

	/* Sega H1 games */
	DRIVER( coolridr )	/* (c) 1995 */

	/* Sega ST-V games */
	DRIVER( stvbios )
	DRIVER( astrass )	/* (c) 1998 Sunsoft */
	DRIVER( bakubaku )	/* (c) 1996 */
	DRIVER( colmns97 )	/* (c) 1996 */
	DRIVER( cotton2 )	/* (c) 1997 Success */
	DRIVER( cottonbm )	/* (c) 1998 Success */
	DRIVER( decathlt )	/* (c) 1995 */
	DRIVER( diehard )	/* (c) 1996 */
	DRIVER( dnmtdeka )	/* (c) 1996 */
	DRIVER( ejihon )	/* (c) 1995 */
	DRIVER( elandore )	/* (c) 1998 Sai-Mate */
	DRIVER( ffreveng )	/* (c) 1999 Capcom */
	DRIVER( fhboxers )	/* (c) 1995 */
	DRIVER( findlove )	/* (c) 1996 Daika / FCF */
	DRIVER( finlarch )	/* (c) 1995 */
	DRIVER( gaxeduel )	/* (c) 1994 */
	DRIVER( grdforce )	/* (c) 1998 Success */
	DRIVER( groovef )	/* (c) 1996 Atlus */
	DRIVER( hanagumi )	/* (c) 1998 */
	DRIVER( introdon )	/* (c) 1996 Sunsoft / Success */
	DRIVER( kiwames )	/* (c) 1995 Athena */
	DRIVER( maruchan )	/* (c) 1992 Sega / Toyosuisan */
	DRIVER( myfairld )	/* (c) 1998 Micronet */
	DRIVER( othellos )	/* (c) 1998 Success */
	DRIVER( pblbeach )	/* (c) 1995 T&E Soft */
	DRIVER( prikura )	/* (c) 1996 Atlus */
	DRIVER( puyosun )	/* (c) 1996 Compile */
	DRIVER( rsgun )		/* (c) 1998 Treasure */
	DRIVER( sandor )	/* (c) 1995 */
	DRIVER( thunt )		/* (c) 1995 Sega (Deniam License) */
	DRIVER( sassisu )	/* (c) 1992 */
	DRIVER( seabass )	/* (c) 1998 A wave Inc (Able License) */
	DRIVER( shanhigw )	/* (c) 1996 */
	DRIVER( shienryu )	/* (c) 1997 Warashi */
	DRIVER( smleague )	/* (c) 1995 */
	DRIVER( sokyugrt )	/* (c) 1996 Raizing / 8ing */
	DRIVER( sss )		/* (c) 1998 Capcom / Cave / Victor */
	DRIVER( suikoenb )	/* (c) 1995 Data East */
	DRIVER( twcup98 )	/* (c) 1998 Techmo */
	DRIVER( vfkids )	/* (c) 1995 */
	DRIVER( vfremix )	/* (c) 1996 */
	DRIVER( vmahjong )	/* (c) 1997 Micronet */
	DRIVER( winterht )	/* (c) 1997 */
	DRIVER( znpwfv )	/* (c) 1997 */

	DRIVER( danchih )	/* (c) 1999 Altron (Tecmo License) */
	DRIVER( mausuke )	/* (c) 1995 Data East */
	DRIVER( batmanfr )	/* (c) 1996 Acclaim */

	DRIVER( critcrsh )	/* (c) 1995 Sega */

	DRIVER( sfish2 )	/* (c) 1995 */
	DRIVER( sfish2j )	/* (c) 1995 */

	/* Sega Model 2 Games */

	/* Model 2 (TGPs, Model 1 sound board) */
	DRIVER( daytona )	/* 1993 */
	DRIVER( daytonat )	/* 1993, hack */
	DRIVER( daytonam )	/* 1993, hack */
	DRIVER( desert )	/* 1994 */
	DRIVER( vcop )		/* 1994 */

	/* Model 2A-CRX (TGPs, SCSP sound board) */
	DRIVER( manxtt )	/* 1995 */
	DRIVER( srallyc )	/* 1995 */
	DRIVER( vf2 )		/* 1995 */
	DRIVER( vf2b )		/* 1995 */
	DRIVER( vf2o )		/* 1995 */
	DRIVER( vcop2 )		/* 1995 */
	DRIVER( dynamcop )	/* 1996 */
	DRIVER( dyndeka2 )	/* 1996 */
	DRIVER( doaa )		/* 1996 */
	DRIVER( zeroguna )	/* 1997 */
	DRIVER( pltkidsa )	/* 1998 */

	/* Model 2B-CRX (SHARC, SCSP sound board) */
	DRIVER( vstriker )	/* 1994 */
	DRIVER( vstrikra )	/* 1994 */
	DRIVER( fvipers )	/* 1995 */
	DRIVER( gunblade )	/* 1995 */
	DRIVER( indy500 )	/* 1995 */
	DRIVER( skytargt )	/* 1995 */
	DRIVER( dynmcopb )	/* 1996 */
	DRIVER( schamp )	/* 1996 */
	DRIVER( lastbrnx )	/* 1996 */
	DRIVER( lastbrnj )	/* 1996 */
	DRIVER( doa )		/* 1996 */
	DRIVER( sgt24h )	/* 1996 */
	DRIVER( von )		/* 1996 */
	DRIVER( vonusa )	/* 1996 */
	DRIVER( dynabb )	/* 1996 */
	DRIVER( zerogun )	/* 1997 */
	DRIVER( zerogunj )	/* 1997 */
	DRIVER( pltkids )	/* 1998 */

	/* Model 2C-CRX (TGPx4, SCSP sound board) */
	DRIVER( skisuprg )	/* 1996 */
	DRIVER( stcc )		/* 1996 */
	DRIVER( hotd )		/* 1997 */
	DRIVER( overrev )	/* 1997 */
	DRIVER( topskatr )	/* 1997 */
	DRIVER( bel )		/* 1998 */

	/* Sega Model 3 Games */

	/* Step 1.0 */
	DRIVER( vf3 )		/* 1996 */
	DRIVER( vf3tb )		/* 1996 */
	DRIVER( bass )		/* 1997 */
	DRIVER( getbass )	/* 1997 */

	/* Step 1.5 */
	DRIVER( scud )		/* 1996 */
	DRIVER( scuda )		/* 1996 */
	DRIVER( scudp )		/* 1997 */
	DRIVER( lostwsga )	/* 1997 */
	DRIVER( vs215 )		/* 1997 */
	DRIVER( lemans24 )	/* 1997 */
	DRIVER( vs29815 )	/* 1998 */

	/* Step 2.0 */
	DRIVER( vs2 )		/* 1997 */
	DRIVER( harley )	/* 1997 */
	DRIVER( srally2 )	/* 1998 */
	DRIVER( sraly2dx )	/* 1998 */
	DRIVER( von2 )		/* 1998 */
	DRIVER( von254g )	/* 1998 */
	DRIVER( fvipers2 )	/* 1998 */
	DRIVER( vs298 )		/* 1998 */
	DRIVER( vs2v991 )	/* 1999 */
	DRIVER( vs299 )		/* 1999 */

	/* Step 2.1 */
	DRIVER( daytona2 )	/* 1998 */
	DRIVER( dayto2pe )	/* 1998 */
	DRIVER( dirtdvls )	/* 1998 */
	DRIVER( dirtdvla )	/* 1998 */
	DRIVER( swtrilgy )	/* 1998 */
	DRIVER( swtrilga )	/* 1998 */
	DRIVER( spikeout )	/* 1998 */
	DRIVER( spikeofe )	/* 1998 */
	DRIVER( eca )		/* 1999 */


	/* Deniam games */
	/* they run on Sega System 16 video hardware */
	DRIVER( logicpro )	/* (c) 1996 Deniam */
	DRIVER( croquis )	/* (c) 1996 Deniam (Germany) */
	DRIVER( karianx )	/* (c) 1996 Deniam */
	DRIVER( logicpr2 )	/* (c) 1997 Deniam (Japan) */


/*
Deniam is a Korean company (http://deniam.co.kr).

Game list:
Title            System     Date
---------------- ---------- ----------
GO!GO!           deniam-16b 1995/10/11
Logic Pro        deniam-16b 1996/10/20
Karian Cross     deniam-16b 1997/04/17
LOTTERY GAME     deniam-16c 1997/05/21
Logic Pro 2      deniam-16c 1997/06/20
Propose          deniam-16c 1997/06/21
BOMULEUL CHAJARA SEGA ST-V  1997/04/11
*/

	/* System C games */
	DRIVER( bloxeedc )	/* (c) 1989 Sega / Elorg */
	DRIVER( bloxeedu )	/* (c) 1989 Sega / Elorg */
	DRIVER( columns )	/* (c) 1990 Sega */
	DRIVER( columnsu )	/* (c) 1990 Sega */
	DRIVER( columnsj )	/* (c) 1990 Sega */
	DRIVER( columns2 )	/* (c) 1990 Sega */
	DRIVER( column2j )	/* (c) 1990 Sega */

	/* System C-2 games */
	DRIVER( borench )	/* (c) 1990 Sega */
	DRIVER( tfrceac )	/* (c) 1990 Sega / Technosoft */
	DRIVER( tfrceacj )	/* (c) 1990 Sega / Technosoft */
	DRIVER( tfrceacb )	/* bootleg */
	DRIVER( twinsqua )	/* (c) 1991 Sega */
	DRIVER( ribbit )	/* (c) 1991 Sega */
	DRIVER( tantr )		/* (c) 1992 Sega */
	DRIVER( tantrkor )	/* (c) 1992 Sega */
	DRIVER( tantrbl )	/* bootleg */
	DRIVER( tantrbl2 )	/* bootleg */
	DRIVER( puyo )		/* (c) 1992 Sega / Compile */
	DRIVER( puyobl )	/* bootleg */
	DRIVER( puyoj )		/* (c) 1992 Sega / Compile */
	DRIVER( puyoja )	/* (c) 1992 Sega / Compile */
	DRIVER( ichir )		/* (c) 1994 Sega */
	DRIVER( ichirk )	/* (c) 1994 Sega */
	DRIVER( ichirj )	/* (c) 1994 Sega */
	DRIVER( ichirjbl )	/* bootleg */
	DRIVER( stkclmns )	/* (c) 1994 Sega */
	DRIVER( stkclmnj )	/* (c) 1994 Sega */
	DRIVER( puyopuy2 )	/* (c) 1994 Compile + Sega license */
	DRIVER( potopoto )	/* (c) 1994 Sega */
	DRIVER( zunkyou )	/* (c) 1994 Sega */

	/* Atlus Print Club 'Games' C-2 board */
	DRIVER( pclubj )
	DRIVER( pclubjv2 )
	DRIVER( pclubjv4 )
	DRIVER( pclubjv5 )

	/* Genie Hardware (uses Genesis VDP) also has 'Sun Mixing Co' put into tile ram */
	DRIVER( puckpkmn )	/* (c) 2000 Genie */
	DRIVER( topshoot )	/* (c) 1995 Sun Mixing */

	/* Sega MegaTech, the number shown is on the label of the instruction rom */
 	DRIVER( megatech )
	DRIVER( mt_beast )	/* 01 */
	DRIVER( mt_shar2 )	/* 02 */
	DRIVER( mt_stbld )	/* 03 */
	DRIVER( mt_ggolf )	/* 04 */
	DRIVER( mt_gsocr )	/* 05 */
						/* 06 */
	DRIVER( mt_asyn )	/* 07 */
	DRIVER( mt_shnbi )	/* 08 */
						/* 09 */
	DRIVER( mt_aftrb )	/* 10 */
	DRIVER( mt_tfor2 )	/* 11 */
						/* 12 */
	DRIVER( mt_astro )	/* 13 */
						/* 14 */
						/* 15 */
						/* 16 */
						/* 17 */
						/* 18 */
						/* 19 */
	DRIVER( mt_lastb )	/* 20 */
	DRIVER( mt_wcsoc )	/* 21 */
	DRIVER( mt_tetri )	/* 22 */
	DRIVER( mt_gng )	/* 23 */
	DRIVER( mt_shang )	/* 24 */
	DRIVER( mt_gaxe )	/* 25 */
						/* 26 */
	DRIVER( mt_mystd )	/* 27 */
	DRIVER( mt_revsh )	/* 28 */
	DRIVER( mt_parlg )	/* 29 */
						/* 30 */
	DRIVER( mt_tgolf )	/* 31 */
						/* 32 */
						/* 33 */
						/* 34 */
	DRIVER( mt_tlbba )	/* 35 */
	DRIVER( mt_cols )	/* 36 */
						/* 37 */
	DRIVER( mt_eswat )	/* 38 */
	DRIVER( mt_smgp )	/* 39 */
	DRIVER( mt_mwalk )	/* 40 */
	DRIVER( mt_crack )	/* 41 */
						/* 42 */
						/* 43 */
	DRIVER( mt_arrow )	/* 44 */
						/* 45 */
						/* 46 */
	DRIVER( mt_astrm )	/* 47 */
						/* 48 */
	DRIVER( mt_bbros )	/* 49 */
						/* 50 */
						/* 51 */
	DRIVER( mt_sonic )	/* 52 */
	DRIVER( mt_sonia )	/* ^^ */
	DRIVER( mt_fshrk )	/* 53 */
						/* 54 */
						/* 55 */
						/* 56 */
	DRIVER( mt_gaxe2 )	/* 57 */
	DRIVER( mt_stf )	/* 58 */
						/* 59 */
	DRIVER( mt_kcham )	/* 60 */
						/* 61 */
	DRIVER( mt_soni2 )	/* 62 */

	/* Sega MegaPlay */
	DRIVER( megaplay )
	DRIVER( mp_sonic )	/* 01 */
	DRIVER( mp_gaxe2 )	/* 02 */
	DRIVER( mp_gslam )	/* 03 */
	DRIVER( mp_twc )	/* 04 */
	DRIVER( mp_sor2 )	/* 05 */
	DRIVER( mp_bio )	/* 06 */
	DRIVER( mp_soni2 )	/* 07 */
						/* 08 */
	DRIVER( mp_shnb3 )	/* 09 */
						/* 10 */
	DRIVER( mp_mazin )	/* 11 */
	DRIVER( mp_col3 )	/* ?? */

	/* Data East "Burger Time hardware" games */
	DRIVER( lnc )		/* (c) 1981 */
	DRIVER( zoar )		/* (c) 1982 */
	DRIVER( btime )		/* (c) 1982 */
	DRIVER( btime2 )	/* (c) 1982 */
	DRIVER( btimem )	/* (c) 1982 + Midway */
	DRIVER( cookrace )	/* bootleg */
	DRIVER( wtennis )	/* bootleg 1982 */
	DRIVER( brubber )	/* (c) 1982 */
	DRIVER( bnj )		/* (c) 1982 + Midway */
	DRIVER( caractn )	/* bootleg */
	DRIVER( disco )		/* (c) 1982 */
	DRIVER( discof )	/* (c) 1982 */
	DRIVER( sdtennis )	/* (c) 1983 */
	DRIVER( mmonkey )	/* (c) 1982 Technos Japan + Roller Tron */

		/* cassette system, parent is decocass */
	DRIVER( decocass )
	DRIVER( ctsttape )	/* ? */
	DRIVER( cterrani )	/* 04 (c) 1981 */
	DRIVER( castfant )	/* 07 (c) 1981 */
	DRIVER( csuperas )	/* 09 (c) 1981 */
	DRIVER( clocknch )	/* 11 (c) 1981 */
	DRIVER( cprogolf )	/* 13 (c) 1981 */
	DRIVER( cluckypo )	/* 15 (c) 1981 */
	DRIVER( ctisland )	/* 16 (c) 1981 */
	DRIVER( ctislnd2 )	/* 16 (c) 1981 */
	DRIVER( ctislnd3 )	/* 16? (c) 1981 */
	DRIVER( cdiscon1 )	/* 19 (c) 1982 */
	DRIVER( csweetht )	/* ?? (c) 1982, clone of disco no 1 */
	DRIVER( ctornado )	/* 20 (c) 1982 */
	DRIVER( cmissnx )	/* 21 (c) 1982 */
	DRIVER( cptennis )	/* 22 (c) 1982 */
	DRIVER( cexplore )	/* ?? (c) 1982 */
	DRIVER( cbtime )	/* 26 (c) 1982 */
	DRIVER( cburnrub )	/* ?? (c) 1982 */
	DRIVER( cburnrb2 )	/* ?? (c) 1982 */
	DRIVER( cbnj )		/* 27 (c) 1982 */
	DRIVER( cgraplop )	/* 28 (c) 1983 */
	DRIVER( cgraplp2 )	/* 28? (c) 1983 */
	DRIVER( clapapa )	/* 29 (c) 1983 */
	DRIVER( clapapa2 )	/* 29 (c) 1983 */ /* this one doesn't display lapapa anyehere */
	DRIVER( cnightst )	/* 32 (c) 1983 */
	DRIVER( cnights2 )	/* 32 (c) 1983 */
	DRIVER( cprosocc )	/* 33 (c) 1983 */
	DRIVER( cprobowl )	/* ?? (c) 1983 */
	DRIVER( cscrtry )	/* 38 (c) 1984 */
	DRIVER( cscrtry2 )	/* 38 (c) 1984 */
	DRIVER( cppicf )	/* 39 (c) 1984 */
	DRIVER( cppicf2 )	/* 39 (c) 1984 */
	DRIVER( cfghtice )	/* 40 (c) 1984 */
	DRIVER( cbdash )	/* 44 (c) 1985 */
		/* the following don't work at all */
	DRIVER( chwy )		/* ?? (c) 198? */
	DRIVER( cflyball )	/* ?? (c) 198? */
	DRIVER( czeroize )	/* ?? (c) 198? */

	/* other Data East games */
	DRIVER( madalien )	/* (c) 1980 */
	DRIVER( astrof )	/* (c) [1980?] */
	DRIVER( astrof2 )	/* (c) [1980?] */
	DRIVER( astrof3 )	/* (c) [1980?] */
	DRIVER( abattle )	/* Sidam */
	DRIVER( abattle2 )	/* Sidam */
	DRIVER( afire )		/* Rene Pierre */
	DRIVER( acombat )	/* bootleg */
	DRIVER( sstarbtl )	/* bootleg */
	DRIVER( tomahawk )	/* (c) [1980?] */
	DRIVER( tomahaw5 )	/* (c) [1980?] */
	DRIVER( prosoccr )	/* (c) 1983 */
	DRIVER( prosport )	/* (c) 1983 */
	DRIVER( boomrang )	/* (c) 1983 */
	DRIVER( kamikcab )	/* (c) 1984 */
	DRIVER( yellowcb )	/* bootleg */
	DRIVER( liberate )	/* (c) 1984  Data East Corporation */
	DRIVER( dualaslt )	/* (c) 1984  Data East USA (US) */
	DRIVER( liberatb )	/* bootleg */
	DRIVER( bwing )		/* (c) 1984 */
	DRIVER( bwings )	/* (c) 1984 */
	DRIVER( batwings )	/* (c) 1984 */
	DRIVER( zaviga )	/* (c) */
	DRIVER( zavigaj )	/* (c) */
	DRIVER( kchamp )	/* (c) 1984 Data East USA (US) */
	DRIVER( karatedo )	/* (c) 1984 Data East Corporation (Japan) */
	DRIVER( kchampvs )	/* (c) 1984 Data East USA (US) */
	DRIVER( karatevs )	/* (c) 1984 Data East Corporation (Japan) */
	DRIVER( compgolf )	/* (c) 1985 Data East Corporation (Japan) */
	DRIVER( compglfo )	/* (c) 1985 Data East Corporation (Japan) */
	DRIVER( cntsteer )	/* (c) 1985 Data East Corporation */
	DRIVER( zerotrgt )	/* (c) 1985 Data East Corporation */
	DRIVER( gekitsui )	/* (c) 1985 Data East Corporation (Japan) */
	DRIVER( tryout )	/* (c) 1985 Data East Corporation (Japan) */
	DRIVER( firetrap )	/* (c) 1986 Data East USA (US) */
	DRIVER( firetpbl )	/* bootleg */
	DRIVER( metlclsh )	/* (c) 1985 Data East */
	DRIVER( brkthru )	/* (c) 1986 Data East USA (US) */
	DRIVER( brkthruj )	/* (c) 1986 Data East Corporation (Japan) */
	DRIVER( forcebrk )	/* (c) 1986 Data East Corporation (Japan) */
	DRIVER( darwin )	/* (c) 1986 Data East Corporation (Japan) */
	DRIVER( shootout )	/* (c) 1985 Data East USA (US) */
	DRIVER( shootouj )	/* (c) 1985 Data East USA (Japan) */
	DRIVER( shootoub )	/* bootleg */
	DRIVER( sidepckt )	/* (c) 1986 Data East Corporation */
	DRIVER( sidepctj )	/* (c) 1986 Data East Corporation */
	DRIVER( sidepctb )	/* bootleg */
	DRIVER( exprraid )	/* (c) 1986 Data East USA (US) */
	DRIVER( exprrada )	/* (c) 1986 Data East USA (US) */
	DRIVER( wexpress )	/* (c) 1986 Data East Corporation (World?) */
	DRIVER( wexpresb )	/* bootleg */
	DRIVER( wexpresc )	/* bootleg */
	DRIVER( pcktgal )	/* (c) 1987 Data East Corporation (Japan) */
	DRIVER( pcktgalb )	/* bootleg */
	DRIVER( pcktgal2 )	/* (c) 1989 Data East Corporation (World?) */
	DRIVER( spool3 )	/* (c) 1989 Data East Corporation (World?) */
	DRIVER( spool3i )	/* (c) 1990 Data East Corporation + I-Vics license */
	DRIVER( battlera )	/* (c) 1988 Data East Corporation (World) */
	DRIVER( bldwolf )	/* (c) 1988 Data East USA (US) */
	DRIVER( actfancr )	/* (c) 1989 Data East Corporation (World) */
	DRIVER( actfanc1 )	/* (c) 1989 Data East Corporation (World) */
	DRIVER( actfancj )	/* (c) 1989 Data East Corporation (Japan) */
	DRIVER( triothep )	/* (c) 1989 Data East Corporation (World) */
	DRIVER( triothej )	/* (c) 1989 Data East Corporation (Japan) */

	/* Data East 8-bit games */
	DRIVER( lastmisn )	/* (c) 1986 Data East USA (US) */
	DRIVER( lastmsno )	/* (c) 1986 Data East USA (US) */
	DRIVER( lastmsnj )	/* (c) 1986 Data East Corporation (Japan) */
	DRIVER( shackled )	/* (c) 1986 Data East USA (US) */
	DRIVER( breywood )	/* (c) 1986 Data East Corporation (Japan) */
	DRIVER( csilver )	/* (c) 1987 Data East Corporation (Japan) */
	DRIVER( ghostb )	/* (c) 1987 Data East USA (US) */
	DRIVER( ghostb3 )	/* (c) 1987 Data East USA (US) */
	DRIVER( meikyuh )	/* (c) 1987 Data East Corporation (Japan) */
	DRIVER( srdarwin )	/* (c) 1987 Data East Corporation (World) */
	DRIVER( srdarwnj )	/* (c) 1987 Data East Corporation (Japan) */
	DRIVER( gondo )		/* (c) 1987 Data East USA (US) */
	DRIVER( makyosen )	/* (c) 1987 Data East Corporation (Japan) */
	DRIVER( garyoret )	/* (c) 1987 Data East Corporation (Japan) */
	DRIVER( cobracom )	/* (c) 1988 Data East Corporation (World) */
	DRIVER( cobracmj )	/* (c) 1988 Data East Corporation (Japan) */
	DRIVER( oscar )		/* (c) 1988 Data East USA (US) */
	DRIVER( oscarj )	/* (c) 1987 Data East Corporation (Japan) */
	DRIVER( oscarj1 )	/* (c) 1987 Data East Corporation (Japan) */
	DRIVER( oscarj0 )	/* (c) 1987 Data East Corporation (Japan) */

	/* Data East 16-bit games */
	DRIVER( karnov )	/* (c) 1987 Data East USA (US) */
	DRIVER( karnovj )	/* (c) 1987 Data East Corporation (Japan) */
	DRIVER( wndrplnt )	/* (c) 1987 Data East Corporation (Japan) */
	DRIVER( chelnov )	/* (c) 1988 Data East USA (World) */
	DRIVER( chelnovu )	/* (c) 1988 Data East USA (US) */
	DRIVER( chelnovj )	/* (c) 1988 Data East Corporation (Japan) */
		/* the following ones all run on similar hardware */
	DRIVER( hbarrel )	/* (c) 1987 Data East USA (US) */
	DRIVER( hbarrelw )	/* (c) 1987 Data East Corporation (World) */
	DRIVER( baddudes )	/* (c) 1988 Data East USA (US) */
	DRIVER( drgninja )	/* (c) 1988 Data East Corporation (Japan) */
	DRIVER( birdtry )	/* (c) 1988 Data East Corporation (Japan) */
	DRIVER( robocop )	/* (c) 1988 Data East Corporation (World) */
	DRIVER( robocopw )	/* (c) 1988 Data East Corporation (World) */
	DRIVER( robocopj )	/* (c) 1988 Data East Corporation (Japan) */
	DRIVER( robocopu )	/* (c) 1988 Data East USA (US) */
	DRIVER( robocpu0 )	/* (c) 1988 Data East USA (US) */
	DRIVER( robocopb )	/* bootleg */
	DRIVER( hippodrm )	/* (c) 1989 Data East USA (US) */
	DRIVER( ffantasy )	/* (c) 1989 Data East Corporation (Japan) */
	DRIVER( ffantasa )	/* (c) 1989 Data East Corporation (Japan) */
	DRIVER( slyspy )	/* (c) 1989 Data East USA (US) */
	DRIVER( slyspy2 )	/* (c) 1989 Data East USA (US) */
	DRIVER( secretag )	/* (c) 1989 Data East Corporation (World) */
	DRIVER( secretab )	/* bootleg */
	DRIVER( midres )	/* (c) 1989 Data East Corporation (World) */
	DRIVER( midresu )	/* (c) 1989 Data East USA (US) */
	DRIVER( midresj )	/* (c) 1989 Data East Corporation (Japan) */
	DRIVER( bouldash )	/* (c) 1990 Data East Corporation (World) */
	DRIVER( bouldshj )	/* (c) 1990 Data East Corporation (Japan) */
		/* end of similar hardware */
	DRIVER( stadhero )	/* (c) 1988 Data East Corporation (Japan) */
	DRIVER( madmotor )	/* (c) [1989] Mitchell */
		/* All these games have a unique code stamped on the mask roms */
	DRIVER( vaportra )	/* MAA (c) 1989 Data East Corporation (World) */
	DRIVER( vaportru )	/* MAA (c) 1989 Data East Corporation (US) */
	DRIVER( kuhga )		/* MAA (c) 1989 Data East Corporation (Japan) */
	DRIVER( cbuster )	/* MAB (c) 1990 Data East Corporation (World) */
	DRIVER( cbusterw )	/* MAB (c) 1990 Data East Corporation (World) */
	DRIVER( cbusterj )	/* MAB (c) 1990 Data East Corporation (Japan) */
	DRIVER( twocrude )	/* MAB (c) 1990 Data East USA (US) */
	DRIVER( darkseal )	/* MAC (c) 1990 Data East Corporation (World) */
	DRIVER( darksea1 )	/* MAC (c) 1990 Data East Corporation (World) */
	DRIVER( darkseaj )	/* MAC (c) 1990 Data East Corporation (Japan) */
	DRIVER( gatedoom )	/* MAC (c) 1990 Data East Corporation (US) */
	DRIVER( gatedom1 )	/* MAC (c) 1990 Data East Corporation (US) */
	DRIVER( edrandy )	/* MAD (c) 1990 Data East Corporation (World) */
	DRIVER( edrandy1 )	/* MAD (c) 1990 Data East Corporation (World) */
	DRIVER( edrandyj )	/* MAD (c) 1990 Data East Corporation (Japan) */
	DRIVER( supbtime )	/* MAE (c) 1990 Data East Corporation (World) */
	DRIVER( supbtimj )	/* MAE (c) 1990 Data East Corporation (Japan) */
	DRIVER( mutantf )	/* MAF (c) 1992 Data East Corporation (World) */
	DRIVER( mutantfa )	/* MAF (c) 1992 Data East Corporation (World) */
	DRIVER( deathbrd )	/* MAF (c) 1992 Data East Corporation (Japan) */
	DRIVER( cninja )	/* MAG (c) 1991 Data East Corporation (World) */
	DRIVER( cninja0 )	/* MAG (c) 1991 Data East Corporation (World) */
	DRIVER( cninjau )	/* MAG (c) 1991 Data East Corporation (US) */
	DRIVER( joemac )	/* MAG (c) 1991 Data East Corporation (Japan) */
	DRIVER( stoneage )	/* bootleg */
	DRIVER( robocop2 )	/* MAH (c) 1991 Data East Corporation (World) */
	DRIVER( robocp2u )	/* MAH (c) 1991 Data East Corporation (US) */
	DRIVER( robocp2j )	/* MAH (c) 1991 Data East Corporation (Japan) */
	DRIVER( thndzone )	/* MAJ (c) 1991 Data East Corporation (World) */
	DRIVER( dassault )	/* MAJ (c) 1991 Data East Corporation (US) */
	DRIVER( dassaul4 )	/* MAJ (c) 1991 Data East Corporation (US) */
	DRIVER( chinatwn )	/* MAK (c) 1991 Data East Corporation (Japan) */
	DRIVER( rohga )		/* MAM (c) 1991 Data East Corporation (Asia/Euro) */
	DRIVER( rohga1 )	/* MAM (c) 1991 Data East Corporation (Asia/Euro) */
	DRIVER( rohga2 )	/* MAM (c) 1991 Data East Corporation (Asia/Euro) */
	DRIVER( rohgah )	/* MAM (c) 1991 Data East Corporation (Hong Kong) */
	DRIVER( rohgau )	/* MAM (c) 1991 Data East Corporation (US) */
	DRIVER( schmeisr )	/*     (c) 1993 Hot B */
	DRIVER( captaven )	/* MAN (c) 1991 Data East Corporation (Asia) */
	DRIVER( captavna )	/* MAN (c) 1991 Data East Corporation (Asia) */
	DRIVER( captavne )	/* MAN (c) 1991 Data East Corporation (UK) */
	DRIVER( captavnu )	/* MAN (c) 1991 Data East Corporation (US) */
	DRIVER( captavuu )	/* MAN (c) 1991 Data East Corporation (US) */
	DRIVER( captavnj )	/* MAN (c) 1991 Data East Corporation (Japan) */
				/* MAO ?? */
	DRIVER( tumblep )	/* MAP (c) 1991 Data East Corporation (World) */
	DRIVER( tumblepj )	/* MAP (c) 1991 Data East Corporation (Japan) */
	DRIVER( tumbleb )	/* bootleg */
	DRIVER( tumbleb2 )	/* bootleg */
	DRIVER( jumpkids )	/* (c) 1993 Comad */
	DRIVER( jumppop )	/* (c) 2001 ESD */
	DRIVER( pangpang )	/* (c) 1994 Dong Gue La Mi Ltd. */
	DRIVER( fncywld )	/* (c) 1996 Unico */
	DRIVER( htchctch )	/* (c) 1995 SemiCom */
	DRIVER( cookbib )	/* (c) 1995 SemiCom */
	DRIVER( chokchok )	/* (c) 1995 SemiCom */
	DRIVER( metlsavr )	/* (c) 1994 */
	DRIVER( bcstry )	/* (c) 199? SemiCom */
	DRIVER( bcstrya )	/* (c) 199? SemiCom */
	DRIVER( semibase )	/* (c) 199? SemiCom */
	DRIVER( suprtrio )	/* (c) 1994 GameAce */
	DRIVER( lemmings )	/* prototype (c) 1991 Data East USA (US) */
				/* MAQ ?? */
	DRIVER( dragngun )	/* MAR (c) 1992 Data East Corporation (US) */
	DRIVER( wizdfire )	/* MAS (c) 1992 Data East Corporation (US) */
	DRIVER( darksel2 )	/* MAS (c) 1992 Data East Corporation (Japan) */
	DRIVER( funkyjet )	/* MAT (c) 1992 Mitchell */
	DRIVER( funkyjej )	/* MAT (c) 1992 Mitchell */
				/* MAU ?? */
	DRIVER( nitrobal )	/* MAV (c) 1992 Data East Corporation (US) */
	DRIVER( gunball )	/* MAV (c) 1992 Data East Corporation (Japan) */
				/* MAW ?? */
				/* MAX ?? */
	DRIVER( dietgo )	/* MAY (c) 1993 */
	DRIVER( dietgoe )	/* MAY (c) 1993 */
	DRIVER( dietgou )	/* MAY (c) 1993 */
	DRIVER( dietgoj )	/* MAY (c) 1993 */
	DRIVER( pktgaldx )	/* MAZ (c) 1993 Data East (Euro v3.00) */
	DRIVER( pktgaldj )	/* MAZ (c) 1993 */
	DRIVER( pktgaldb )	/* bootleg */
				/* MBA ?? */
				/* MBB ?? */
				/* MBC ?? */
	DRIVER( boogwing )	/* MBD (c) 1992 */
	DRIVER( boogwina )	/* MBD (c) 1992 */
	DRIVER( ragtime )	/* MBD (c) 1992 */
	DRIVER( dblewing )	/* MBE (c) 1993 Mitchell */
	DRIVER( fghthist )	/* MBF (c) 1993 Data East Corporation (World) */
	DRIVER( fghthstu )	/* MBF (c) 1993 Data East Corporation (US) */
	DRIVER( fghthsta )	/* MBF (c) 1993 Data East Corporation (US) */
	DRIVER( hvysmsh )	/* MBG */
	DRIVER( nslasher )	/* MBH (c) 1993 Data East Corporation */
	DRIVER( nslashej )	/* MBH (c) 1993 Data East Corporation (Japan)*/
	DRIVER( nslashes )	/* MBH (c) 1993 Data East Corporation (Japan)*/
				/* MBI ?? */
				/* MBJ ?? */
				/* MBK ?? */
				/* MBL ?? */
	DRIVER( lockload )	/* MBM (c) 1994 Data East Corporation (US) */
	DRIVER( joemacr )	/* MBN (c) 1994 */
	DRIVER( joemacra )	/* MBN (c) 1994 */
				/* MBO ?? */
				/* MBP ?? */
				/* MBQ ?? */
	DRIVER( tattass )	/* prototype (c) 1994 Data East Pinball (US) */
	DRIVER( tattassa )	/* prototype (c) 1994 Data East Pinball (Asia) */
	DRIVER( charlien )	/* MBR (c) 1994 Mitchell */
				/* MBS ?? */
				/* MBT ?? */
				/* MBU ?? */
				/* MBV ?? */
				/* MBW ?? */
	DRIVER( wcvol95 )	/* MBX (c) 1993 Data East */
				/* MBY ?? */
	DRIVER( backfire )	/* MBZ (c) 1995 */
	DRIVER( backfira )	/* MBZ (c) 1995 */
				/* MCA ?? */
	DRIVER( prtytime )	/* MCB (c) 1995 Mitchell */
	DRIVER( gangonta )	/* MCB (c) 1995 Mitchell */
	DRIVER( chainrec )	/* MCC (c) 1994 */
	DRIVER( magdrop )	/* MCC (c) 1995 Data East */
	DRIVER( magdropp )	/* MCC (c) 1995 Data East */
				/* MCD ?? */
				/* MCE Dunk Dream 95/Hoops96 See MLC games below */
	DRIVER( osman )		/* MCF (c) 1996 Mitchell */
	DRIVER( candance )	/* MCF (c) 1996 Mitchell */
	DRIVER( sotsugyo )	/*     (c) 1995 Mitchell (Atlus license) */
	DRIVER( sshangha )	/*     (c) 1992 Hot-B */
	DRIVER( sshanghb )	/* bootleg */

	/* Data East MLC Games */
	DRIVER( hoops96 )	/* MCE (c) 1996 Data East Corporation */
	DRIVER( ddream95 )	/* MCE (c) 1996 Data East Corporation */
	DRIVER( hoops95 )	/* MCE (c) 1996 Data East Corporation */
	DRIVER( avengrgs )	/* MCG (c) 1995 Data East Corporation (World) */
	DRIVER( avengrgj )	/* MCG (c) 1995 Data East Corporation (Japan) */
	DRIVER( skullfng )	/* MCH (c) 1996 Data East Corporation (Japan) */
	DRIVER( stadhr96 )	/* MCM (c) 1996 Data East */
	DRIVER( stadh96a )	/* MCM (c) 1996 Data East */

	/* Tehkan / Tecmo games (Tehkan became Tecmo in 1986) */
	DRIVER( senjyo )	/* (c) 1983 Tehkan */
	DRIVER( starforc )	/* (c) 1984 Tehkan */
	DRIVER( starfore )	/* (c) 1984 Tehkan */
	DRIVER( megaforc )	/* (c) 1985 Tehkan + Video Ware license */
	DRIVER( baluba )	/* (c) 1986 Able Corp. */
	DRIVER( bombjack )	/* (c) 1984 Tehkan */
	DRIVER( bombjac2 )	/* (c) 1984 Tehkan */
	DRIVER( pbaction )	/* (c) 1985 Tehkan */
	DRIVER( pbactio2 )	/* (c) 1985 Tehkan */
	DRIVER( pbactio3 )	/* (c) 1985 Tehkan */
				/* 6009 Tank Busters */
	DRIVER( ponttehk )	/* 6011 - (c) 1985 Tehkan */
	DRIVER( lvpoker )	/* ???? - (c) 1985 Tehkan */
	DRIVER( lvcards )	/* (c) 1985 Tehkan */
	DRIVER( tehkanwc )	/* (c) 1985 Tehkan */
	DRIVER( gridiron )	/* (c) 1985 Tehkan */
	DRIVER( teedoff )	/* 6102 - (c) 1986 Tecmo */
	DRIVER( solomon )	/* (c) 1986 Tecmo */
	DRIVER( rygar )		/* 6002 - (c) 1986 Tecmo */
	DRIVER( rygar2 )	/* 6002 - (c) 1986 Tecmo */
	DRIVER( rygarj )	/* 6002 - (c) 1986 Tecmo */
	DRIVER( gemini )	/* (c) 1987 Tecmo */
	DRIVER( silkworm )	/* 6217 - (c) 1988 Tecmo */
	DRIVER( silkwrm2 )	/* 6217 - (c) 1988 Tecmo */
	DRIVER( tbowl )		/* 6206 - (c) 1987 Tecmo */
	DRIVER( tbowlj )	/* 6206 - (c) 1987 Tecmo */
	DRIVER( shadoww )	/* 6215 - (c) 1988 Tecmo (World) */
	DRIVER( shadowwa )	/* 6215 - (c) 1988 Tecmo (World) */
	DRIVER( gaiden )	/* 6215 - (c) 1988 Tecmo (US) */
	DRIVER( ryukendn )	/* 6215 - (c) 1989 Tecmo (Japan) */
	DRIVER( wildfang )	/* (c) 1989 Tecmo */
	DRIVER( tknight )	/* (c) 1989 Tecmo */
	DRIVER( stratof )	/* (c) 1991 Tecmo */
	DRIVER( raiga )		/* (c) 1991 Tecmo */
	DRIVER( drgnbowl )	/* (c) 1992 Nics */
	DRIVER( wc90 )		/* (c) 1989 Tecmo */
	DRIVER( wc90a )		/* (c) 1989 Tecmo */
	DRIVER( wc90t )		/* (c) 1989 Tecmo */
	DRIVER( wc90b )		/* bootleg */
	DRIVER( spbactn )	/* 9002 - (c) 1991 Tecmo */
	DRIVER( spbactnj )	/* 9002 - (c) 1991 Tecmo */
	DRIVER( fstarfrc )	/* (c) 1992 Tecmo */
	DRIVER( ginkun )	/* (c) 1995 Tecmo */
	DRIVER( deroon )
	DRIVER( tkdensho )

	/* Konami bitmap games */
	DRIVER( tutankhm )	/* GX350 (c) 1982 Konami */
	DRIVER( tutankst )	/* GX350 (c) 1982 Stern */
	DRIVER( junofrst )	/* GX310 (c) 1983 Konami */
	DRIVER( junofstg )	/* GX310 (c) 1983 Konami + Gottlieb license */

	/* Konami games */
	DRIVER( pooyan )	/* GX320 (c) 1982 */
	DRIVER( pooyans )	/* GX320 (c) 1982 Stern */
	DRIVER( pootan )	/* bootleg */
	DRIVER( timeplt )	/* GX393 (c) 1982 */
	DRIVER( timepltc )	/* GX393 (c) 1982 + Centuri license*/
	DRIVER( timeplta )	/* GX393 (c) 1982 + Atari license*/
	DRIVER( spaceplt )	/* bootleg */
	DRIVER( psurge )	/* (c) 1988 unknown (NOT Konami) */
	DRIVER( megazone )	/* GX319 (c) 1983 */
	DRIVER( megazona )	/* GX319 (c) 1983 */
	DRIVER( megazonb )	/* GX319 (c) 1983 + Kosuka */
	DRIVER( megazonc )	/* GX319 (c) 1983 + Kosuka */
	DRIVER( megaznik )	/* GX319 (c) 1983 + Interlogic / Kosuka */
	DRIVER( pandoras )	/* GX328 (c) 1984 + Interlogic */
	DRIVER( gyruss )	/* GX347 (c) 1983 */
	DRIVER( gyrussce )	/* GX347 (c) 1983 + Centuri license */
	DRIVER( venus )		/* bootleg */
	DRIVER( trackfld )	/* GX361 (c) 1983 */
	DRIVER( trackflc )	/* GX361 (c) 1983 + Centuri license */
	DRIVER( hyprolym )	/* GX361 (c) 1983 */
	DRIVER( hyprolyb )	/* bootleg */
	DRIVER( atlantol )	/* bootleg */
	DRIVER( wizzquiz )	/* (c) 1985 Konami */
	DRIVER( wizzquza )	/* (c) 1985 Zilec-Zenitone */
	DRIVER( mastkin )	/* (c) 1988 Du Tech */
	DRIVER( rocnrope )	/* GX364 (c) 1983 */
	DRIVER( rocnropk )	/* GX364 (c) 1983 + Kosuka */
	DRIVER( circusc )	/* GX380 (c) 1984 */
	DRIVER( circusc2 )	/* GX380 (c) 1984 */
	DRIVER( circusc3 )	/* GX380 (c) 1984 */
	DRIVER( circuscc )	/* GX380 (c) 1984 + Centuri license */
	DRIVER( circusce )	/* GX380 (c) 1984 + Centuri license */
	DRIVER( tp84 )		/* GX388 (c) 1984 */
	DRIVER( tp84a )		/* GX388 (c) 1984 */
	DRIVER( tp84b )		/* GX388 (c) 1984 */
	DRIVER( hyperspt )	/* GX330 (c) 1984 + Centuri */
	DRIVER( hpolym84 )	/* GX330 (c) 1984 */
	DRIVER( sbasketb )	/* GX405 (c) 1984 */
	DRIVER( sbasketo )	/* GX405 (c) 1984 */
	DRIVER( sbasketu )	/* GX405 (c) 1984 */
	DRIVER( mikie )		/* GX469 (c) 1984 */
	DRIVER( mikiej )	/* GX469 (c) 1984 */
	DRIVER( mikiehs )	/* GX469 (c) 1984 */
	DRIVER( roadf )		/* GX461 (c) 1984 */
	DRIVER( roadf2 )	/* GX461 (c) 1984 */
	DRIVER( yiear )		/* GX407 (c) 1985 */
	DRIVER( yiear2 )	/* GX407 (c) 1985 */
	DRIVER( kicker )	/* GX477 (c) 1985 */
	DRIVER( shaolins )	/* GX477 (c) 1985 */
	DRIVER( pingpong )	/* GX555 (c) 1985 */
	DRIVER( merlinmm )	/* (c) 1986 Zilec-Zenitone */
	DRIVER( gberet )	/* GX577 (c) 1985 */
	DRIVER( rushatck )	/* GX577 (c) 1985 */
	DRIVER( gberetb )	/* bootleg on different hardware */
	DRIVER( mrgoemon )	/* GX621 (c) 1986 (Japan) */
	DRIVER( jailbrek )	/* GX507 (c) 1986 */
	DRIVER( manhatan )	/* GX507 (c) 1986 (Japan) */
	DRIVER( scotrsht )	/* GX545 (c) 1985 */
	DRIVER( finalizr )	/* GX523 (c) 1985 */
	DRIVER( finalizb )	/* bootleg */
	DRIVER( ironhors )	/* GX560 (c) 1986 */
	DRIVER( dairesya )	/* GX560 (c) 1986 (Japan) */
	DRIVER( farwest )
	DRIVER( jackal )	/* GX631 (c) 1986 (World) */
	DRIVER( topgunr )	/* GX631 (c) 1986 (US) */
	DRIVER( jackalj )	/* GX631 (c) 1986 (Japan) */
	DRIVER( topgunbl )	/* bootleg */
	DRIVER( ddribble )	/* GX690 (c) 1986 */
	DRIVER( contra )	/* GX633 (c) 1987 */
	DRIVER( contrab )	/* bootleg */
	DRIVER( contraj )	/* GX633 (c) 1987 (Japan) */
	DRIVER( contrajb )	/* bootleg */
	DRIVER( gryzor )	/* GX633 (c) 1987 */
	DRIVER( combasc )	/* GX611 (c) 1988 */
	DRIVER( combasct )	/* GX611 (c) 1987 */
	DRIVER( combascj )	/* GX611 (c) 1987 (Japan) */
	DRIVER( bootcamp )	/* GX611 (c) 1987 */
	DRIVER( combascb )	/* bootleg */
	DRIVER( rockrage )	/* GX620 (c) 1986 (World?) */
	DRIVER( rockraga )	/* GX620 (c) 1986 (Prototype?) */
	DRIVER( rockragj )	/* GX620 (c) 1986 (Japan) */
	DRIVER( mx5000 )	/* GX669 (c) 1987 */
	DRIVER( flkatck )	/* GX669 (c) 1987 (Japan) */
	DRIVER( fastlane )	/* GX752 (c) 1987 */
	DRIVER( tricktrp )	/* GX771 (c) 1987 */
	DRIVER( labyrunr )	/* GX771 (c) 1987 (Japan) */
	DRIVER( labyrunk )	/* GX771 (c) 1987 (Japan) */
	DRIVER( thehustl )	/* GX765 (c) 1987 (Japan) */
	DRIVER( thehustj )	/* GX765 (c) 1987 (Japan) */
	DRIVER( rackemup )	/* GX765 (c) 1987 */
	DRIVER( battlnts )	/* GX777 (c) 1987 */
	DRIVER( battlntj )	/* GX777 (c) 1987 (Japan) */
	DRIVER( bladestl )	/* GX797 (c) 1987 */
	DRIVER( bladstle )	/* GX797 (c) 1987 */
	DRIVER( hcastle )	/* GX768 (c) 1988 */
	DRIVER( hcastleo )	/* GX768 (c) 1988 */
	DRIVER( hcastlej )	/* GX768 (c) 1988 (Japan) */
	DRIVER( hcastljo )	/* GX768 (c) 1988 (Japan) */
	DRIVER( ajax )		/* GX770 (c) 1987 */
	DRIVER( typhoon )	/* GX770 (c) 1987 */
	DRIVER( ajaxj )		/* GX770 (c) 1987 (Japan) */
	DRIVER( scontra )	/* GX775 (c) 1988 */
	DRIVER( scontraj )	/* GX775 (c) 1988 (Japan) */
	DRIVER( thunderx )	/* GX873 (c) 1988 */
	DRIVER( thnderxa )	/* GX873 (c) 1988 */
	DRIVER( thnderxb )	/* GX873 (c) 1988 */
	DRIVER( thnderxj )	/* GX873 (c) 1988 (Japan) */
	DRIVER( mainevt )	/* GX799 (c) 1988 */
	DRIVER( mainevto )	/* GX799 (c) 1988 */
	DRIVER( mainev2p )	/* GX799 (c) 1988 */
	DRIVER( ringohja )	/* GX799 (c) 1988 (Japan) */
	DRIVER( devstors )	/* GX890 (c) 1988 */
	DRIVER( devstor2 )	/* GX890 (c) 1988 */
	DRIVER( devstor3 )	/* GX890 (c) 1988 */
	DRIVER( garuka )	/* GX890 (c) 1988 (Japan) */
	DRIVER( 88games )	/* GX861 (c) 1988 */
	DRIVER( konami88 )	/* GX861 (c) 1988 */
	DRIVER( hypsptsp )	/* GX861 (c) 1988 (Japan) */
	DRIVER( gbusters )	/* GX878 (c) 1988 */
	DRIVER( crazycop )	/* GX878 (c) 1988 (Japan) */
	DRIVER( crimfght )	/* GX821 (c) 1989 (US) */
	DRIVER( crimfgt2 )	/* GX821 (c) 1989 (World) */
	DRIVER( crimfgtj )	/* GX821 (c) 1989 (Japan) */
	DRIVER( spy )		/* GX857 (c) 1989 (World) */
	DRIVER( spyu )		/* GX857 (c) 1989 (US) */
	DRIVER( bottom9 )	/* GX891 (c) 1989 */
	DRIVER( bottom9n )	/* GX891 (c) 1989 */
	DRIVER( mstadium )	/* GX891 (c) 1989 (Japan) */
	DRIVER( blockhl )	/* GX973 (c) 1989 */
	DRIVER( quarth )	/* GX973 (c) 1989 (Japan) */
	DRIVER( aliens )	/* GX875 (c) 1990 (World) */
	DRIVER( aliens2 )	/* GX875 (c) 1990 (World) */
	DRIVER( aliens3 )	/* GX875 (c) 1990 (World) */
	DRIVER( aliensu )	/* GX875 (c) 1990 (US) */
	DRIVER( aliensj )	/* GX875 (c) 1990 (Japan) */
	DRIVER( aliensj2 )	/* GX875 (c) 1990 (Japan) */
	DRIVER( suratk )	/* GX911 (c) 1990 (World) */
	DRIVER( suratka )	/* GX911 (c) 1990 (Asia) */
	DRIVER( suratkj )	/* GX911 (c) 1990 (Japan) */
	DRIVER( parodius )	/* GX955 (c) 1990 (World) */
	DRIVER( parodisj )	/* GX955 (c) 1990 (Japan) */
	DRIVER( rollerg )	/* GX999 (c) 1991 (US) */
	DRIVER( rollergj )	/* GX999 (c) 1991 (Japan) */
	DRIVER( simpsons )	/* GX072 (c) 1991 */
	DRIVER( simps4pa )	/* GX072 (c) 1991 */
	DRIVER( simpsn2p )	/* GX072 (c) 1991 */
	DRIVER( simps2pa )	/* GX072 (c) 1991 */
	DRIVER( simps2pj )	/* GX072 (c) 1991 (Japan) */
	DRIVER( esckids )	/* GX975 (c) 1991 (Asia) */
	DRIVER( esckidsj )	/* GX975 (c) 1991 (Japan) */
	DRIVER( vendetta )	/* GX081 (c) 1991 (World) */
	DRIVER( vendetao )	/* GX081 (c) 1991 (World) */
	DRIVER( vendet2p )	/* GX081 (c) 1991 (World) */
	DRIVER( vendetas )	/* GX081 (c) 1991 (Asia) */
	DRIVER( vendtaso )	/* GX081 (c) 1991 (Asia) */
	DRIVER( vendettj )	/* GX081 (c) 1991 (Japan) */
	DRIVER( mogura )	/* GX141 (c) 1991 */
	DRIVER( wecleman )	/* GX602 (c) 1986 */
	DRIVER( hotchase )	/* GX763 (c) 1988 */
	DRIVER( chqflag )	/* GX717 (c) 1988 */
	DRIVER( chqflagj )	/* GX717 (c) 1988 (Japan) */
	DRIVER( ultraman )	/* GX910 (c) 1991 Banpresto/Bandai */
	DRIVER( hexion )	/* GX122 (c) 1992 */
	DRIVER( lethalen )	/* GX191 (c) 1992 */
	DRIVER( lethalej )	/* GX191 (c) 1992 */

	/* Konami "Nemesis hardware" games */
	DRIVER( nemesis )	/* GX456 (c) 1985 */
	DRIVER( nemesuk )	/* GX456 (c) 1985 */
	DRIVER( konamigt )	/* GX561 (c) 1985 */
	DRIVER( salamand )	/* GX587 (c) 1986 */
	DRIVER( salamanj )	/* GX587 (c) 1986 */
	DRIVER( lifefrce )	/* GX587 (c) 1986 (US) */
	DRIVER( lifefrcj )	/* GX587 (c) 1986 (Japan) */
	DRIVER( blkpnthr )	/* GX604 (c) 1987 (Japan) */
	DRIVER( citybomb )	/* GX787 (c) 1987 (World) */
	DRIVER( citybmrj )	/* GX787 (c) 1987 (Japan) */
	DRIVER( kittenk )	/* GX712 (c) 1988 */
	DRIVER( nyanpani )	/* GX712 (c) 1988 (Japan) */
	DRIVER( hcrash )	/* GX790 (c) 1987 */
	DRIVER( hcrashc )	/* GX790 (c) 1987 */

	/* GX400 BIOS based games */
	DRIVER( rf2 )		/* GX561 (c) 1985 */
	DRIVER( twinbee )	/* GX412 (c) 1985 */
	DRIVER( gradius )	/* GX456 (c) 1985 */
	DRIVER( gwarrior )	/* GX578 (c) 1985 */

	/* Konami "Twin 16" games */
	DRIVER( devilw )	/* GX687 (c) 1987 */
	DRIVER( darkadv )	/* GX687 (c) 1987 */
	DRIVER( majuu )		/* GX687 (c) 1987 (Japan) */
	DRIVER( vulcan )	/* GX785 (c) 1988 */
	DRIVER( gradius2 )	/* GX785 (c) 1988 (Japan) */
	DRIVER( grdius2a )	/* GX785 (c) 1988 (Japan) */
	DRIVER( grdius2b )	/* GX785 (c) 1988 (Japan) */
	DRIVER( cuebrick )	/* GX903 (c) 1989 */
	DRIVER( fround )	/* GX870 (c) 1988 */
	DRIVER( froundl )	/* GX870 (c) 1988 */
	DRIVER( hpuncher )	/* GX870 (c) 1988 (Japan) */
	DRIVER( miaj )		/* GX808 (c) 1989 (Japan) */

	/* (some) Konami 68000 games */
	DRIVER( cuebrckj )	/* GX903 (c) 1989 */
	DRIVER( mia )		/* GX808 (c) 1989 */
	DRIVER( mia2 )		/* GX808 (c) 1989 */
	DRIVER( tmnt )		/* GX963 (c) 1989 (World) */
	DRIVER( tmntu )		/* GX963 (c) 1989 (US) */
	DRIVER( tmntua )	/* GX963 (c) 1989 (US) */
	DRIVER( tmht )		/* GX963 (c) 1989 (UK) */
	DRIVER( tmntj )		/* GX963 (c) 1990 (Japan) */
	DRIVER( tmht2p )	/* GX963 (c) 1989 (UK) */
	DRIVER( tmnt2pj )	/* GX963 (c) 1990 (Japan) */
	DRIVER( tmnt2po )	/* GX963 (c) 1989 (Oceania) */
	DRIVER( punkshot )	/* GX907 (c) 1990 (US) */
	DRIVER( punksht2 )	/* GX907 (c) 1990 (US) */
	DRIVER( punkshtj )	/* GX907 (c) 1990 (Japan) */
	DRIVER( lgtnfght )	/* GX939 (c) 1990 (World) */
	DRIVER( lgtnfghu )	/* GX939 (c) 1990 (US) */
	DRIVER( trigon )	/* GX939 (c) 1990 (Japan) */
	DRIVER( blswhstl )	/* GX060 (c) 1991 */
	DRIVER( detatwin )	/* GX060 (c) 1991 (Japan) */
	DRIVER( glfgreat )	/* GX061 (c) 1991 */
	DRIVER( glfgretj )	/* GX061 (c) 1991 (Japan) */
	DRIVER( tmnt2 )		/* GX063 (c) 1991 (US) */
	DRIVER( tmnt22p )	/* GX063 (c) 1991 (US) */
	DRIVER( tmnt2a )	/* GX063 (c) 1991 (Asia) */
	DRIVER( ssriders )	/* GX064 (c) 1991 (World) */
	DRIVER( ssrdrebd )	/* GX064 (c) 1991 (World) */
	DRIVER( ssrdrebc )	/* GX064 (c) 1991 (World) */
	DRIVER( ssrdruda )	/* GX064 (c) 1991 (US) */
	DRIVER( ssrdreaa )	/* GX064 (c) 1991 (US) */
	DRIVER( ssrdruac )	/* GX064 (c) 1991 (US) */
	DRIVER( ssrdrubc )	/* GX064 (c) 1991 (US) */
	DRIVER( ssrdrabd )	/* GX064 (c) 1991 (Asia) */
	DRIVER( ssrdradd )	/* GX064 (c) 1991 (Asia) */
	DRIVER( ssrdrjbd )	/* GX064 (c) 1991 (Japan) */
	DRIVER( sunsetbl )	/* bootleg */
	DRIVER( xmen )		/* GX065 (c) 1992 (US) */
	DRIVER( xmen2p )	/* GX065 (c) 1992 (World) */
	DRIVER( xmen2pj )	/* GX065 (c) 1992 (Japan) */
	DRIVER( xmen6p )	/* GX065 (c) 1992 */
	DRIVER( xmen6pu )	/* GX065 (c) 1992 */
	DRIVER( xexex )		/* GX067 (c) 1991 (World) */
	DRIVER( xexexa )	/* GX067 (c) 1991 (AAA) */
	DRIVER( xexexj )	/* GX067 (c) 1991 (Japan) */
	DRIVER( asterix )	/* GX068 (c) 1992 (World) */
	DRIVER( astrxeac )	/* GX068 (c) 1992 (World) */
	DRIVER( astrxeaa )	/* GX068 (c) 1992 (World) */
	DRIVER( gijoe )		/* GX069 (c) 1991 (World) */
	DRIVER( gijoeu )	/* GX069 (c) 1991 (US) */
	DRIVER( gijoej )	/* GX069 (c) 1991 (Japan) */
	DRIVER( thndrx2 )	/* GX073 (c) 1991 (Japan) */
	DRIVER( thndrx2a )	/* GX073 (c) 1991 (Asia) */
	DRIVER( prmrsocr )	/* GX101 (c) 1993 (Europe) */
	DRIVER( prmrsocj )	/* GX101 (c) 1993 (Japan) */
	DRIVER( qgakumon )	/* GX248 (c) 1993 (Japan) */
	DRIVER( moo )		/* GX151 (c) 1992 (World) */
	DRIVER( mooua )		/* GX151 (c) 1992 (US) */
	DRIVER( mooaa )		/* GX151 (c) 1992 (US) */
	DRIVER( moobl )		/* bootleg */
	DRIVER( bucky )		/* GX173 (c) 1992 (World) */
	DRIVER( buckyua )	/* GX173 (c) 1992 (US) */
	DRIVER( gaiapols )	/* GX123 (c) 1993 (Europe) */
	DRIVER( gaiapolu )	/* GX123 (c) 1993 (US) */
	DRIVER( gaiapolj )	/* GX123 (c) 1993 (Japan) */
	DRIVER( mystwarr )	/* GX128 (c) 1993 (World) */
	DRIVER( mystwaru )	/* GX128 (c) 1993 (US) */
	DRIVER( viostorm )	/* GX168 (c) 1993 (Europe) */
	DRIVER( viostrmu )	/* GX168 (c) 1993 (US) */
	DRIVER( viostrmj )	/* GX168 (c) 1993 (Japan) */
	DRIVER( viostrma )	/* GX168 (c) 1993 (Asia) */
	DRIVER( mmaulers )	/* GX170 (c) 1993 (Europe) */
	DRIVER( dadandrn )	/* GX170 (c) 1993 (Japan) */
	DRIVER( metamrph )	/* GX224 (c) 1993 (Europe) */
	DRIVER( metamrpu )	/* GX224 (c) 1993 (US) */
	DRIVER( metamrpj )	/* GX224 (c) 1993 (Japan) */
	DRIVER( mtlchamp )	/* GX234 (c) 1993 (World) */
	DRIVER( mtlchmp1 )	/* GX234 (c) 1993 (World) */
	DRIVER( mtlchmpu )	/* GX234 (c) 1993 (US) */
	DRIVER( mtlchmpj )	/* GX234 (c) 1993 (Japan) */
	DRIVER( mtlchmpa )	/* GX234 (c) 1993 (Japan) */
	DRIVER( rungun )	/* GX247 (c) 1993 (World) */
	DRIVER( runguna )	/* GX247 (c) 1993 (World) */
	DRIVER( rungunu )	/* GX247 (c) 1993 (US) */
	DRIVER( rungunua )	/* GX247 (c) 1993 (US) */
	DRIVER( slmdunkj )	/* GX247 (c) 1993 (Japan) */
	DRIVER( dbz )		/* (c) 1993 Banpresto */
	DRIVER( dbz2 )		/* (c) 1994 Banpresto */
	DRIVER( bishi )		/* GX575 (c) 1996 (Japan) */
	DRIVER( sbishi )	/* GX675 (c) 1998 (Japan) */

	/* Konami dual 68000 games */
	DRIVER( overdriv )	/* GX789 (c) 1990 */
	DRIVER( gradius3 )	/* GX945 (c) 1989 (Japan) */
	DRIVER( grdius3a )	/* GX945 (c) 1989 (Asia) */
	DRIVER( grdius3e )	/* GX945 (c) 1989 (World?) */

	/* Konami 68020 games */
	DRIVER( plygonet )	/* GX305 (c) 1993 */

	/* Konami System GX games */
	DRIVER( konamigx )

	/* GX Type 1 */
	DRIVER( racinfrc )	/* GX250 (c) 1994 */
	DRIVER( opengolf )	/* GX218 (c) 1994 */
	DRIVER( opengol2 )	/* GX218 (c) 1994 */
	DRIVER( ggreats2 )	/* GX218 (c) 1994 */

	/* GX Type 2 */
	DRIVER( le2 )		/* GX312 (c) 1994 */
	DRIVER( le2u )		/* GX312 (c) 1994 */
	DRIVER( le2j )		/* GX312 (c) 1994 */
	DRIVER( puzldama )	/* GX315 (c) 1994 (Japan) */
	DRIVER( gokuparo )	/* GX321 (c) 1994 (Japan) */
	DRIVER( fantjour )	/* GX321 (c) 1994 */
	DRIVER( dragoona )	/* GX417 (c) 1995 (Asia) */
	DRIVER( dragoonj )	/* GX417 (c) 1995 (Japan) */
	DRIVER( tbyahhoo )	/* GX424 (c) 1995 (Japan) */
	DRIVER( tkmmpzdm )	/* GX515 (c) 1995 (Japan) */
	DRIVER( salmndr2 )	/* GX521 (c) 1996 (Japan) */
	DRIVER( sexyparo )	/* GX533 (c) 1996 (Japan) */
	DRIVER( daiskiss )	/* GX535 (c) 1996 (Japan) */
	DRIVER( tokkae )	/* GX615 (c) 1996 (Japan) */
	DRIVER( winspike )	/* GX705 (c) 1997 EAA */
	DRIVER( winspikj )	/* GX705 (c) 1997 JAA */

	/* GX Type 3 */
	DRIVER( soccerss )	/* GX427 (c) 1994 */
	DRIVER( soccersj )	/* GX427 (c) 1994 */
	DRIVER( soccersa )	/* GX427 (c) 1994 */

	/* GX Type 4 */
	DRIVER( vsnetscr )	/* GX627 (c) 1996 */
	DRIVER( vsnetscj )	/* GX627 (c) 1996 */
	DRIVER( rungun2 )	/* GX505 (c) 1996 */
	DRIVER( slamdnk2 )	/* GX505 (c) 1996 */
	DRIVER( rushhero )	/* GX605 (c) 1996 */

	/* 68k + HD */
	DRIVER( qdrmfgp )	/* GQ460 (c) 1994 (Japan) */
	DRIVER( qdrmfgp2 )	/* GE557 (c) 1995 (Japan) */

	/* DJ Main */
	DRIVER( bm1stmix )	/* GQ753 (c) 1997 (Japan) */
	DRIVER( bm2ndmix )	/* GX853 (c) 1998 (Japan) */
	DRIVER( bm2ndmxa )	/* GX853 (c) 1998 (Japan) */
	DRIVER( bmcompmx )	/* GX858 (c) 1999 (Japan) */
	DRIVER( hmcompmx )	/* GX858 (c) 1999 */
	DRIVER( bm4thmix )	/* GX847 (c) 1999 (Japan) */
	DRIVER( bm5thmix )	/* GX981 (c) 1999 (Japan */
	DRIVER( bmcorerm )	/* GXA05 (c) 1999 (Japan) */
	DRIVER( bmdct )		/* GX995 (c) 2000 (Japan) */
	DRIVER( bmcompm2 )	/* GX988 (c) 2000 (Japan) */
	DRIVER( hmcompm2 )	/* GX988 (c) 2000 */
	DRIVER( bm6thmix )	/* GXA21 (c) 2001 (Japan) */

	/* PowerPC based Konami games */
	DRIVER( thunderh )	/* GX680 (c)1996 */
	DRIVER( slrasslt )	/* GX792 (c)1996 */
	DRIVER( gticlub )	/* GX688 (c)1996 */
	DRIVER( gticlubj )	/* GX688 (c)1996 */
	DRIVER( hangplt )	/* GX685 (c)1997 */
	DRIVER( nbapbp )	/* GX778 (c)1998 */
	DRIVER( gradius4 )	/* GX837 (c)1999 */
	DRIVER( sscope )	/* GX830 (c)1999 */
	DRIVER( sscopea )	/* GX830 (c)1999 */
	DRIVER( sscope2 )	/* GX931 (c)2000 */
	DRIVER( racingj )	/* GX676 (c)1998 */
	DRIVER( racingj2 )	/* GX888 (c)1999 */
	DRIVER( thrilld )	/* GX713 (c)1998 */
	DRIVER( midnrun )	/* GX??? (c)1995 */
	DRIVER( windheat )	/* GX677 (c)1996 */

	/* Konami M2 games */
	DRIVER( polystar )	/* GX623 (c)1997 */
	DRIVER( totlvice )	/* ? (c)1997 */
	DRIVER( btltryst )	/* GX636 (c)1998 */
	DRIVER( heatof11 )	/* GX703 (c)1998 */
	DRIVER( evilngt )	/* GX810 (c)1998 */

	/* Exidy games */
	DRIVER( carpolo )	/* (c) 1977 */
	DRIVER( sidetrac )	/* (c) 1979 */
	DRIVER( targ )		/* (c) 1980 */
	DRIVER( targc )		/* (c) 1980 */
	DRIVER( spectar )	/* (c) 1980 */
	DRIVER( spectar1 )	/* (c) 1980 */
	DRIVER( rallys )	/* (c) 1980 Novar (bootleg?) */
	DRIVER( phantoma )	/* (c) 1980 Jeutel (bootleg?) */
	DRIVER( venture )	/* (c) 1981 */
	DRIVER( venture2 )	/* (c) 1981 */
	DRIVER( venture4 )	/* (c) 1981 */
	DRIVER( mtrap )		/* (c) 1981 */
	DRIVER( mtrap3 )	/* (c) 1981 */
	DRIVER( mtrap4 )	/* (c) 1981 */
	DRIVER( pepper2 )	/* (c) 1982 */
	DRIVER( hardhat )	/* (c) 1982 */
	DRIVER( fax )		/* (c) 1983 */
	DRIVER( faxa )		/* (c) 1983 */
	DRIVER( circus )	/* no copyright notice [1977?] */
	DRIVER( robotbwl )	/* no copyright notice */
	DRIVER( crash )		/* Exidy [1979?] */
	DRIVER( ripcord )	/* Exidy [1977?] */
	DRIVER( starfire )	/* Exidy [1979?] */
	DRIVER( starfira )	/* Exidy [1979?] */
	DRIVER( fireone )	/* (c) 1979 Exidy */
	DRIVER( starfir2 )	/* (c) 1979 Exidy */
	DRIVER( victory )	/* (c) 1982 */
	DRIVER( victorba )	/* (c) 1982 */
	DRIVER( topgunnr )	/* (c) 1986 */

	/* Exidy 440 games */
	DRIVER( crossbow )	/* (c) 1983 */
	DRIVER( cheyenne )	/* (c) 1984 */
	DRIVER( combat )	/* (c) 1985 */
	DRIVER( catch22 )	/* (c) 1985 */
	DRIVER( cracksht )	/* (c) 1985 */
	DRIVER( claypign )	/* (c) 1986 */
	DRIVER( chiller )	/* (c) 1986 */
	DRIVER( topsecex )	/* (c) 1986 */
	DRIVER( hitnmiss )	/* (c) 1987 */
	DRIVER( hitnmis2 )	/* (c) 1987 */
	DRIVER( whodunit )	/* (c) 1988 */
	DRIVER( showdown )	/* (c) 1988 */

	/* Exidy Max-A-Flex */
	DRIVER( maxaflex )
	DRIVER( mf_achas )
	DRIVER( mf_brist )
	DRIVER( mf_flip )
	DRIVER( mf_bdash )

	/* Atari b/w games */
	DRIVER( tank8 )		/* ??????           1976/04 [6800] */
	DRIVER( tank8a )	/* ??????           1976/04 [6800] */
	DRIVER( tank8b )	/* ??????           1976/04 [6800] */
	DRIVER( tank8c )	/* ??????           1976/04 [6800] */
	DRIVER( tank8d )	/* ??????           1976/04 [6800] */
	DRIVER( cball )		/* ?????? prototype 1976/?? [6800] */
	DRIVER( copsnrob )	/* 005625           1976/07 [6502] */
	DRIVER( flyball )	/* 005629           1976/07 [6502] */
	DRIVER( sprint2 )	/* 005922           1976/11 [6502] */
	DRIVER( sprint2a )	/* 005922           1976/11 [6502] */
	DRIVER( sprint4 )	/* 008716           1977/12 [6502] */
	DRIVER( sprint4a )	/* 008716           1977/12 [6502] */
	DRIVER( nitedrvr )	/* 006321           1976/10 [6502] */
	DRIVER( dominos )	/* 007305           1977/01 [6502] */
	DRIVER( triplhnt )	/* 008422-008791    1977/04 [6800] */
	DRIVER( sprint8 )	/* ??????           1977/05 [6800] */
	DRIVER( sprint8a )	/* ??????           1977/05 [6800] */
	DRIVER( dragrace )	/* 008505-008521    1977/06 [6800] */
	DRIVER( poolshrk )	/* 006281           1977/06 [6800] */
	DRIVER( starshp1 )	/* 007513-007531    1977/07 [6502] */
	DRIVER( starshpp )	/* 007513-007531    1977/07 [6502] */
	DRIVER( superbug )	/* 009115-009467    1977/09 [6800] */
	DRIVER( canyon )	/* 009493-009504    1977/10 [6502] */
	DRIVER( canyonp )	/* 009493-009504    1977/10 [6502] */
	DRIVER( destroyr )	/* 030131-030136    1977/10 [6800] */
	DRIVER( sprint1 )	/* 006443           1978/01 [6502] */
	DRIVER( ultratnk )	/* 009801           1978/02 [6502] */
	DRIVER( skyraid )	/* 009709           1978/03 [6502] */
	DRIVER( tourtabl )	/* 030170           1978/03 [6507] */
	DRIVER( tourtab2 )	/* 030170           1978/03 [6507] */
	DRIVER( avalnche )	/* 030574           1978/04 [6502] */
	DRIVER( cascade )	/* bootleg                         */
	DRIVER( firetrk )	/* 030926           1978/06 [6808] */
	DRIVER( skydiver )	/* 009787           1978/06 [6800] */
	/* Smokey Joe */	/* 030926           1978/07 [6502] */
	DRIVER( sbrkout )	/* 033442-033455    1978/09 [6502] */
	DRIVER( atarifb )	/* 033xxx           1978/10 [6502] */
	DRIVER( atarifb1 )	/* 033xxx           1978/10 [6502] */
	DRIVER( orbit )		/* 033689-033702    1978/11 [6800] */
	DRIVER( boxer )		/* ?????? prototype 1978    [6502] */
	DRIVER( wolfpack )	/* ?????? prototype         [6502] */
	DRIVER( mgolf )		/* ?????? prototype 1978    [6502] */
	DRIVER( videopin )	/* 034253-034267    1979/02 [6502] */
	DRIVER( atarifb4 )	/* 034754           1979/04 [6502] */
	DRIVER( subs )		/* 033714           1979/05 [6502] */
	DRIVER( bsktball )	/* 034756-034766    1979/05 [6502] */
	DRIVER( abaseb )	/* 034711-034738    1979/06 [6502] */
	DRIVER( abaseb2 )	/* 034711-034738    1979/06 [6502] */
	DRIVER( montecar )	/* 035763-035780    1980/04 [6502] */
	DRIVER( soccer )	/* 035222-035260    1980/04 [6502] */

	/* Atari "Missile Command hardware" games */
	DRIVER( missile )	/* 035820-035825    (c) 1980 */
	DRIVER( missile2 )	/* 035820-035825    (c) 1980 */
	DRIVER( suprmatk )	/*                  (c) 1980 + (c) 1981 Gencomp */
	DRIVER( sprmatkd )	/*                  (c) 1980 + (c) 1981 Gencomp */

	/* Atari vector games */
	DRIVER( llander )	/* 0345xx           no copyright notice */
	DRIVER( llander1 )	/* 0345xx           no copyright notice */
	DRIVER( asteroid )	/* 035127-035145    (c) 1979 */
	DRIVER( asteroi1 )	/* 035127-035145    no copyright notice */
	DRIVER( asteroib )	/* (bootleg) */
	DRIVER( asterock )	/* Sidam bootleg    (c) 1979 */
	DRIVER( meteorts )	/* VCC bootleg      (c) 1979 */
	DRIVER( astdelux )	/* 0351xx           (c) 1980 */
	DRIVER( astdelu1 )	/* 0351xx           (c) 1980 */
	DRIVER( bzone )		/* 0364xx           (c) 1980 */
	DRIVER( bzone2 )	/* 0364xx           (c) 1980 */
	DRIVER( bzonec )	/* 0364xx           (c) 1980 */
	DRIVER( bradley )	/*     ??           (c) 1980 */
	DRIVER( redbaron )	/* 036995-037007    (c) 1980 */
	DRIVER( tempest )	/* 136002           (c) 1980 */
	DRIVER( tempest1 )	/* 136002           (c) 1980 */
	DRIVER( tempest2 )	/* 136002           (c) 1980 */
	DRIVER( tempest3 )	/* 136002           (c) 1980 */
	DRIVER( temptube )	/* (hack) */
	DRIVER( spacduel )	/* 136006           (c) 1980 */
	DRIVER( gravitar )	/* 136010           (c) 1982 */
	DRIVER( gravitr2 )	/* 136010           (c) 1982 */
	DRIVER( gravp )		/* (proto)          (c) 1982 */
	DRIVER( lunarbat )	/* (proto)          (c) 1982 */
	DRIVER( lunarba1 )	/* (proto)          (c) 1982 */
	DRIVER( quantum )	/* 136016           (c) 1982 */	/* made by Gencomp */
	DRIVER( quantum1 )	/* 136016           (c) 1982 */	/* made by Gencomp */
	DRIVER( quantump )	/* 136016           (c) 1982 */	/* made by Gencomp */
	DRIVER( bwidow )	/* 136017           (c) 1982 */
	DRIVER( starwars )	/* 136021           (c) 1983 */
	DRIVER( starwar1 )	/* 136021           (c) 1983 */
	DRIVER( mhavoc )	/* 136025           (c) 1983 */
	DRIVER( mhavoc2 )	/* 136025           (c) 1983 */
	DRIVER( mhavocp )	/* 136025           (c) 1983 */
	DRIVER( mhavocrv )	/* (hack) */
	DRIVER( alphaone )	/* (proto)          (c) 1983 */
	DRIVER( alphaona )	/* (proto)          (c) 1983 */
	DRIVER( esb )		/* 136031           (c) 1985 */

	/* Atari "Centipede hardware" games */
	DRIVER( warlords )	/* 037153-037159    (c) 1980 */
	DRIVER( centiped )	/* 136001           (c) 1980 */
	DRIVER( centipd2 )	/* 136001           (c) 1980 */
	DRIVER( centtime )	/* 136001           (c) 1980 */
	DRIVER( centipdb )	/* (bootleg) */
	DRIVER( caterplr )	/* (bootleg) */
	DRIVER( millpac )	/* Valadon */
	DRIVER( magworm )	/* (bootleg) */
	DRIVER( milliped )	/* 136013           (c) 1982 */
	DRIVER( qwak )	 	/* (proto)          (c) 1982 */
	DRIVER( runaway )	/* (proto)          (c) 1982 */
	DRIVER( bullsdrt )	/* (c) 1985 Shinkai */

	/* misc Atari games */
	DRIVER( tunhunt )	/* 136000           (c) 1979 */
	DRIVER( tunhuntc )	/* 136000           (c) 1981 */ /* licensed to / distributed by Centuri */
	DRIVER( liberatr )	/* 136012           (c) 1982 */
	DRIVER( liberat2 )	/* 136012           (c) 1982 */
	DRIVER( foodf )		/* 136020           (c) 1982 */	/* made by Gencomp */
	DRIVER( foodf2 )	/* 136020           (c) 1982 */	/* made by Gencomp */
	DRIVER( foodfc )	/* 136020           (c) 1982 */	/* made by Gencomp */
	DRIVER( ccastles )	/* 136022           (c) 1983 */
	DRIVER( ccastle3 )	/* 136022           (c) 1983 */
	DRIVER( ccastle2 )	/* 136022           (c) 1983 */
	DRIVER( cloak )		/* 136023           (c) 1983 */
	DRIVER( cloaksp )	/* 136023           (c) 1983 */
	DRIVER( cloakfr )	/* 136023           (c) 1983 */
	DRIVER( cloakgr )	/* 136023           (c) 1983 */
	DRIVER( cloud9 )	/* (proto)          (c) 1983 */
	DRIVER( jedi )		/* 136030           (c) 1984 */

	/* Atari System 1 games */
	DRIVER( peterpak )	/* 136028           (c) 1984 */
	/* Marble Madness */
	DRIVER( marble )	/* 136033           (c) 1984 */
	DRIVER( marble2 )	/* 136033           (c) 1984 */
	DRIVER( marble3 )	/* 136033           (c) 1984 */
	DRIVER( marble4 )	/* 136033           (c) 1984 */
	/* Indiana Jones and the Temple of Doom */
	DRIVER( indytemp )	/* 136036           (c) 1985 */
	DRIVER( indytem2 )	/* 136036           (c) 1985 */
	DRIVER( indytem3 )	/* 136036           (c) 1985 */
	DRIVER( indytem4 )	/* 136036           (c) 1985 */
	DRIVER( indytemd )	/* 136036           (c) 1985 */
	/* Road Runner */
	DRIVER( roadrunn )	/* 136040           (c) 1985 */
	DRIVER( roadrun2 )	/* 136040           (c) 1985 */
	DRIVER( roadrun1 )	/* 136040           (c) 1985 */
	/* Road Blasters */
	DRIVER( roadblst )	/* 136048           (c) 1986, 1987 */
	DRIVER( roadblsg )	/* 136048           (c) 1986, 1987 */
	DRIVER( roadbls3 )	/* 136048           (c) 1986, 1987 */
	DRIVER( roadblg2 )	/* 136048           (c) 1986, 1987 */
	DRIVER( roadbls2 )	/* 136048           (c) 1986, 1987 */
	DRIVER( roadblg1 )	/* 136048           (c) 1986, 1987 */
	DRIVER( roadbls1 )	/* 136048           (c) 1986, 1987 */
	DRIVER( roadblsc )	/* 136048           (c) 1986, 1987 */
	DRIVER( roadblcg )	/* 136048           (c) 1986, 1987 */
	DRIVER( roadblc1 )	/* 136048           (c) 1986, 1987 */

	/* Atari System 2 games */
	/* Paperboy */
	DRIVER( paperboy )	/* 136034           (c) 1984 */
	DRIVER( paperbr2 )	/* 136034           (c) 1984 */
	DRIVER( paperbr1 )	/* 136034           (c) 1984 */
	/* Super Sprint */
	DRIVER( ssprint )	/* 136042           (c) 1986 */
	DRIVER( ssprint3 )	/* 136042           (c) 1986 */
	DRIVER( ssprint1 )	/* 136042           (c) 1986 */
	DRIVER( ssprintg )	/* 136042           (c) 1986 */
	DRIVER( sspring1 )	/* 136042           (c) 1986 */
	DRIVER( ssprintf )	/* 136042           (c) 1986 */
	DRIVER( ssprints )	/* 136042           (c) 1986 */
	/* Championship Sprint */
	DRIVER( csprint )	/* 136045           (c) 1986 */
	DRIVER( csprint2 )	/* 136045           (c) 1986 */
	DRIVER( csprint1 )	/* 136045           (c) 1986 */
	DRIVER( csprintg )	/* 136045           (c) 1986 */
	DRIVER( cspring1 )	/* 136045           (c) 1986 */
	DRIVER( csprintf )	/* 136045           (c) 1986 */
	DRIVER( csprints )	/* 136045           (c) 1986 */
	DRIVER( csprins1 )	/* 136045           (c) 1986 */
	/* 720 Degrees */
	DRIVER( 720 )		/* 136047           (c) 1986 */
	DRIVER( 720r3 )		/* 136047           (c) 1986 */
	DRIVER( 720r2 )		/* 136047           (c) 1986 */
	DRIVER( 720r1 )		/* 136047           (c) 1986 */
	DRIVER( 720g )		/* 136047           (c) 1986 */
	DRIVER( 720gr1 )	/* 136047           (c) 1986 */
	/* APB. */
	DRIVER( apb )		/* 136051           (c) 1987 */
	DRIVER( apb6 )		/* 136051           (c) 1987 */
	DRIVER( apb5 )		/* 136051           (c) 1987 */
	DRIVER( apb4 )		/* 136051           (c) 1987 */
	DRIVER( apb3 )		/* 136051           (c) 1987 */
	DRIVER( apb2 )		/* 136051           (c) 1987 */
	DRIVER( apb1 )		/* 136051           (c) 1987 */
	DRIVER( apbg )		/* 136051           (c) 1987 */
	DRIVER( apbf )		/* 136051           (c) 1987 */

	/* Atari polygon games */
	DRIVER( irobot )	/* 136029           (c) 1983 */
	/* Hard Drivin' */
	DRIVER( harddriv )	/* 136052           (c) 1988 */
	DRIVER( harddrvb )	/* 136052           (c) 1988 */
	DRIVER( harddrvg )	/* 136052           (c) 1988 */
	DRIVER( harddrvj )	/* 136052           (c) 1988 */
	DRIVER( harddrb6 )	/* 136052           (c) 1988 */
	DRIVER( harddrj6 )	/* 136052           (c) 1988 */
	DRIVER( harddrb5 )	/* 136052           (c) 1988 */
	DRIVER( harddrg4 )	/* 136052           (c) 1988 */
	DRIVER( harddrv3 )	/* 136052           (c) 1988 */
	DRIVER( harddrv2 )	/* 136052           (c) 1988 */
	DRIVER( harddrv1 )	/* 136052           (c) 1988 */
	/* Hard Drivin' Compact */
	DRIVER( harddrvc )	/* 136068           (c) 1990 */
	DRIVER( harddrcg )	/* 136068           (c) 1990 */
	DRIVER( harddrcb )	/* 136068           (c) 1990 */
	DRIVER( harddrc1 )	/* 136068           (c) 1990 */
	/* Stun Runner */
	DRIVER( stunrun )	/* 136070           (c) 1989 */
	DRIVER( stunrunj )	/* 136070           (c) 1989 */
	DRIVER( stunrun5 )	/* 136070           (c) 1989 */
	DRIVER( stunrune )	/* 136070           (c) 1989 */
	DRIVER( stunrun4 )	/* 136070           (c) 1989 */
	DRIVER( stunrun3 )	/* 136070           (c) 1989 */
	DRIVER( stunrn3e )	/* 136070           (c) 1989 */
	DRIVER( stunrun2 )	/* 136070           (c) 1989 */
	DRIVER( stunrn2e )	/* 136070           (c) 1989 */
	DRIVER( stunrun0 )	/* 136070           (c) 1989 */
	DRIVER( stunrunp )	/* (proto)          (c) 1989 */
	/* Race Drivin' */
	DRIVER( racedriv )	/* 136077           (c) 1990 */
	DRIVER( racedrvb )	/* 136077           (c) 1990 */
	DRIVER( racedrvg )	/* 136077           (c) 1990 */
	DRIVER( racedrv4 )	/* 136077           (c) 1990 */
	DRIVER( racedrb4 )	/* 136077           (c) 1990 */
	DRIVER( racedrg4 )	/* 136077           (c) 1990 */
	DRIVER( racedrv3 )	/* 136077           (c) 1990 */
	DRIVER( racedrv2 )	/* 136077           (c) 1990 */
	DRIVER( racedrv1 )	/* 136077           (c) 1990 */
	DRIVER( racedrb1 )	/* 136077           (c) 1990 */
	DRIVER( racedrg1 )	/* 136077           (c) 1990 */
	/* Race Drivin' Compact */
	DRIVER( racedrvc )	/* 136077           (c) 1990 */
	DRIVER( racedrcb )	/* 136077           (c) 1990 */
	DRIVER( racedrcg )	/* 136077           (c) 1990 */
	DRIVER( racedrc4 )	/* 136077           (c) 1990 */
	DRIVER( racedcb4 )	/* 136077           (c) 1990 */
	DRIVER( racedcg4 )	/* 136077           (c) 1990 */
	DRIVER( racedrc2 )	/* 136077           (c) 1990 */
	DRIVER( racedrc1 )	/* 136077           (c) 1990 */
				/* 136086 - BMX Heat (proto) */
	/* Steel Talons */
	DRIVER( steeltal )	/* 136087           (c) 1990 */
	DRIVER( steeltag )	/* 136087           (c) 1990 */
	DRIVER( steelta1 )	/* 136087           (c) 1990 */
	DRIVER( steeltap )	/* 136087           (c) 1990 */
				/* 136088 - Race Drivin' Panorama */
	/* Street Drivin' */
	DRIVER( strtdriv )	/* 136091 (proto) */
	/* Hard Drivin' Airbourne */
	DRIVER( hdrivair )	/* 136098 (proto) */
	DRIVER( hdrivaip )	/* 136098 (proto) */

	/* later Atari games */

	/* Gauntlet Hardware */
	/* Gauntlet */
	DRIVER( gauntlet )	/* 136037           (c) 1985 */
	DRIVER( gaunts )	/* 136037           (c) 1985 */
	DRIVER( gauntj )	/* 136037           (c) 1985 */
	DRIVER( gauntg )	/* 136037           (c) 1985 */
	DRIVER( gauntj12 )	/* 136037           (c) 1985 */
	DRIVER( gauntr9 )	/* 136037           (c) 1985 */
	DRIVER( gauntgr8 )	/* 136037           (c) 1985 */
	DRIVER( gauntr7 )	/* 136037           (c) 1985 */
	DRIVER( gauntgr6 )	/* 136037           (c) 1985 */
	DRIVER( gauntr5 )	/* 136037           (c) 1985 */
	DRIVER( gauntr4 )	/* 136037           (c) 1985 */
	DRIVER( gauntgr3 )	/* 136037           (c) 1985 */
	DRIVER( gauntr2 )	/* 136037           (c) 1985 */
	DRIVER( gauntr1 )	/* 136037           (c) 1985 */
	/* Gauntlet - 2 Player */
	DRIVER( gaunt2p )	/* 136037           (c) 1985 */
	DRIVER( gaunt2pj )	/* 136037           (c) 1985 */
	DRIVER( gaunt2pg )	/* 136037           (c) 1985 */
	DRIVER( gaun2pr3 )	/* 136037           (c) 1985 */
	DRIVER( gaun2pj2 )	/* 136037           (c) 1985 */
	DRIVER( gaun2pg1 )	/* 136037           (c) 1985 */
	/* Gauntlet 2 */
	DRIVER( gaunt2 )	/* 136043           (c) 1986 */
	DRIVER( gaunt2g )	/* 136043           (c) 1986 */
	/* Gauntlet 2 - 2 Player */
	DRIVER( gaunt22p )	/* 136043           (c) 1986 */
	DRIVER( gaun22p1 )	/* 136043           (c) 1986 */
	DRIVER( gaun22pg )	/* 136043           (c) 1986 */
	/* Vindicators Part II */
	DRIVER( vindctr2 )	/* 136059           (c) 1988 */
	DRIVER( vindc2r2 )	/* 136059           (c) 1988 */
	DRIVER( vindc2r1 )	/* 136059           (c) 1988 */

	/* Other Hardware */

	/* Xybots */
	DRIVER( xybots )	/* 136054           (c) 1987 */
	DRIVER( xybotsg )	/* 136054           (c) 1987 */
	DRIVER( xybotsf )	/* 136054           (c) 1987 */
	DRIVER( xybots1 )	/* 136054           (c) 1987 */
	DRIVER( xybots0 )	/* 136054           (c) 1987 */
	/* Blasteroids */
	DRIVER( blstroid )	/* 136057           (c) 1987 */
	DRIVER( blstroi3 )	/* 136057           (c) 1987 */
	DRIVER( blstroi2 )	/* 136057           (c) 1987 */
	DRIVER( blstroig )	/* 136057           (c) 1987 */
	DRIVER( blsthead )	/* (proto)          (c) 1987 */
	/* Vindicators */
	DRIVER( vindictr )	/* 136059           (c) 1988 */
	DRIVER( vindicte )	/* 136059           (c) 1988 */
	DRIVER( vindictg )	/* 136059           (c) 1988 */
	DRIVER( vindice4 )	/* 136059           (c) 1988 */
	DRIVER( vindict4 )	/* 136059           (c) 1988 */
	DRIVER( vindice3 )	/* 136059           (c) 1988 */
	DRIVER( vindict2 )	/* 136059           (c) 1988 */
	DRIVER( vindict1 )	/* 136059           (c) 1988 */
	/* Toobin */
	DRIVER( toobin )	/* 136061           (c) 1988 */
	DRIVER( toobine )	/* 136061           (c) 1988 */
	DRIVER( toobing )	/* 136061           (c) 1988 */
	DRIVER( toobin2 )	/* 136061           (c) 1988 */
	DRIVER( toobin2e )	/* 136061           (c) 1988 */
	DRIVER( toobin1 )	/* 136061           (c) 1988 */
	/* Cyberball */
	DRIVER( cyberbal )	/* 136064           (c) 1989 */
	DRIVER( cyberba2 )	/* 136064           (c) 1989 */
	DRIVER( cyberbap )	/* 136064           (c) 1989 */
	/* Atari Tetris */
	DRIVER( atetcktl )	/* 136066           (c) 1989 */
	DRIVER( atetckt2 )	/* 136066           (c) 1989 */
	DRIVER( atetris )	/* 136066           (c) 1988 */
	DRIVER( atetrisa )	/* 136066           (c) 1988 */
	DRIVER( atetrisb )	/* (bootleg) */
	DRIVER( atetrsb2 )	/* (bootleg) */
	/* Escape from the Planet of Robot Monsters */
	DRIVER( eprom )		/* 136069           (c) 1989 */
	DRIVER( eprom2 )	/* 136069           (c) 1989 */
	/* Cyberball 2 Player */
	DRIVER( cyberb2p )	/* 136071           (c) 1989 */
	DRIVER( cyberb23 )	/* 136071           (c) 1989 */
	DRIVER( cyberb22 )	/* 136071           (c) 1989 */
	DRIVER( cyberb21 )	/* 136071           (c) 1989 */
	/* Skull and Crossbones */
	DRIVER( skullxbo )	/* 136072           (c) 1989 */
	DRIVER( skullxb4 )	/* 136072           (c) 1989 */
	DRIVER( skullxb3 )	/* 136072           (c) 1989 */
	DRIVER( skullxb2 )	/* 136072           (c) 1989 */
	DRIVER( skullxb1 )	/* 136072           (c) 1989 */
	/* Cyberball Tournament */
	DRIVER( cyberbt )	/* 136073           (c) 1989 */
	DRIVER( cyberbt1 )	/* 136073           (c) 1989 */

	DRIVER( badlands )	/* 136074           (c) 1989 */
	DRIVER( klax )		/* 136075           (c) 1989 */
	DRIVER( klax2 )		/* 136075           (c) 1989 */
	DRIVER( klax3 )		/* 136075           (c) 1989 */
	DRIVER( klaxj )		/* 136075           (c) 1989 (Japan) */
	DRIVER( klaxd )		/* 136075           (c) 1989 (Germany) */
	DRIVER( klaxp1 )	/* prototype */
	DRIVER( klaxp2 )	/* prototype */
	DRIVER( thunderj )	/* 136076           (c) 1990 */

	DRIVER( hydra )		/* 136079           (c) 1990 */
	DRIVER( hydrap )	/* (proto)          (c) 1990 */
	DRIVER( hydrap2 )	/* (proto)          (c) 1990 */
	DRIVER( pitfight )	/* 136081           (c) 1990 */
	DRIVER( pitfigh4 )	/* 136081           (c) 1990 */
	DRIVER( pitfigh3 )	/* 136081           (c) 1990 */
	DRIVER( pitfighj )	/* Japan */
	DRIVER( pitfighb )	/* bootleg */
	DRIVER( rampart )	/* 136082           (c) 1990 */
	DRIVER( ramprt2p )	/* 136082           (c) 1990 */
	DRIVER( rampartj )	/* 136082           (c) 1990 (Japan) */
	DRIVER( shuuz )		/* 136083           (c) 1990 */
	DRIVER( shuuz2 )	/* 136083           (c) 1990 */
				/* 136084 - Marble Man (proto) */
	DRIVER( batman )	/* 136085           (c) 1991 */
	DRIVER( roadriot )	/* 136089           (c) 1991 */
	DRIVER( offtwall )	/* 136090           (c) 1991 */
	DRIVER( offtwalc )	/* 136090           (c) 1991 */
	DRIVER( guardian )	/* 136092           (c) 1992 */
	DRIVER( relief )	/* 136093           (c) 1992 */
	DRIVER( relief2 )	/* 136093           (c) 1992 */
	DRIVER( arcadecl )	/* (proto)          (c) 1992 */
	DRIVER( sparkz )	/* (proto)          (c) 1992 */
	DRIVER( motofren )	/* 136094           (c) 1992 */
	DRIVER( motofrmd )	/* 136094           (c) 1992 */
	DRIVER( motofrft )	/* 136094           (c) 1992 */
	DRIVER( motomdft )	/* 136094           (c) 1992 */
	DRIVER( spclords )	/* 136095           (c) 1992 */
	DRIVER( spclordb )	/* 136095           (c) 1992 */
	DRIVER( spclorda )	/* 136095           (c) 1992 */
	DRIVER( spclordg )	/* 136095           (c) 1992 */
	DRIVER( rrreveng )	/*     ??           (c) 1993 */
	DRIVER( rrrevenp )	/*     ??           (c) 1993 */
	DRIVER( beathead )	/* (proto)          (c) 1993 */
	DRIVER( tmek )		/* 136100           (c) 1994 */
	DRIVER( tmekprot )	/* 136100           (c) 1994 */
	DRIVER( primrage )	/* 136102           (c) 1994 */
	DRIVER( primraga )	/* 136102           (c) 1994 */
	DRIVER( area51 )	/* 136105           (c) 1995 */
	DRIVER( area51t )	/* 136105           (c) 1995 */
	DRIVER( area51a )	/* 136105           (c) 1995 */
	DRIVER( area51mx )	/* 136105           (c) 1998 */
	DRIVER( a51mxr3k )	/*     ??           (c) 1998 */
	DRIVER( maxforce )	/*     ??           (c) 1996 */
	DRIVER( maxf_102 )	/*     ??           (c) 1996 */
	DRIVER( vcircle )	/* (proto)          (c) 1996 */
	DRIVER( a51site4 )	/*     ??           (c) 1998 */

	/* SNK / Rock-ola games */
	DRIVER( sasuke )	/* [1980] Shin Nihon Kikaku (SNK) */
	DRIVER( satansat )	/* (c) 1981 SNK */
	DRIVER( zarzon )	/* (c) 1981 Taito, gameplay says SNK */
	DRIVER( vanguard )	/* (c) 1981 SNK */
	DRIVER( vangrdce )	/* (c) 1981 SNK + Centuri */
	DRIVER( vanguarj )	/* (c) 1981 SNK */
	DRIVER( fantasy )	/* (c) 1981 SNK */
	DRIVER( fantasyu )	/* (c) 1981 Rock-ola */
	DRIVER( fantasyj )	/* (c) 1981 SNK */
	DRIVER( pballoon )	/* (c) 1982 SNK */
	DRIVER( nibbler )	/* (c) 1982 Rock-ola */
	DRIVER( nibblera )	/* (c) 1982 Rock-ola */
	DRIVER( nibblerb )	/* (c) 1982 Rock-ola */
	DRIVER( nibblero )	/* (c) 1982 Olympia */

	/* later SNK games, each game can be identified by PCB code and ROM
    code, the ROM code is the same between versions, and usually based
    upon the Japanese title. */
	DRIVER( lasso )		/*       'WM' (c) 1982 */
	DRIVER( chameleo )	/* (c) 1983 Jaleco */
	DRIVER( wwjgtin )	/* (c) 1984 Jaleco / Casio */
	DRIVER( pinbo )		/* (c) 1984 Jaleco */
	DRIVER( pinboa )	/* (c) 1984 Jaleco */
	DRIVER( pinbos )	/* (c) 1985 Strike */
	DRIVER( joyfulr )	/* A2001      (c) 1983 */
	DRIVER( mnchmobl )	/* A2001      (c) 1983 + Centuri license */
	DRIVER( marvins )	/* A2003      (c) 1983 */
	DRIVER( madcrash )	/* A2005      (c) 1984 */
	DRIVER( vangrd2 )	/*            (c) 1984 */
	DRIVER( jcross )	/* .. */
	DRIVER( mainsnk )	/* fill in */
	DRIVER( canvas )	/*            (c) 1985 */
	DRIVER( sgladiat )	/* A3006      (c) 1984 */
	DRIVER( hal21 )		/*            (c) 1985 */
	DRIVER( hal21j )	/*            (c) 1985 (Japan) */
	DRIVER( aso )		/*            (c) 1985 */
	DRIVER( alphamis )	/*            (c) 1985 */
	DRIVER( tnk3 )		/* A5001      (c) 1985 */
	DRIVER( tnk3j )		/* A5001      (c) 1985 */
	DRIVER( athena )	/*       'UP' (c) 1986 */
	DRIVER( fitegolf )	/*       'GU' (c) 1988 */
	DRIVER( fitegol2 )	/*       'GU' (c) 1988 */
	DRIVER( countryc )	/*       'CC' (c) 1988 */
	DRIVER( ikari )		/* A5004 'IW' (c) 1986 */
	DRIVER( ikarijp )	/* A5004 'IW' (c) 1986 (Japan) */
	DRIVER( ikarijpb )	/* bootleg */
	DRIVER( victroad )	/*            (c) 1986 */
	DRIVER( dogosoke )	/*            (c) 1986 */
	DRIVER( dogosokj )	/* bootleg */
	DRIVER( gwar )		/* A7003 'GV' (c) 1987 */
	DRIVER( gwarj )		/* A7003 'GV' (c) 1987 (Japan) */
	DRIVER( gwara )		/* A7003 'GV' (c) 1987 */
	DRIVER( gwarb )		/* bootleg */
	DRIVER( bermudat )	/* A6003 'WW' (c) 1987 */
	DRIVER( bermudao )	/* A6003 'WW' (c) 1987 */
	DRIVER( bermudaa )	/* A6003 'WW' (c) 1987 */
	DRIVER( worldwar )	/* A6003 'WW' (c) 1987 */
	DRIVER( psychos )	/*       'PS' (c) 1987 */
	DRIVER( psychosj )	/*       'PS' (c) 1987 (Japan) */
	DRIVER( chopper )	/* A7003 'KK' (c) 1988 */
	DRIVER( choppera )	/* A7003 'KK' (c) 1988 */
	DRIVER( chopperb )	/* A7003 'KK' (c) 1988 */
	DRIVER( legofair )	/* A7003 'KK' (c) 1988 */
	DRIVER( ftsoccer )	/*            (c) 1988 */
	DRIVER( tdfever )	/* A6006 'TD' (c) 1987 */
	DRIVER( tdfeverj )	/* A6006 'TD' (c) 1987 */
	DRIVER( tdfever2 )	/* A6006 'TD'?(c) 1988 */
	DRIVER( ikari3 )	/* A7007 'IK3'(c) 1989 */
	DRIVER( ikari3nr )	/* A7007 'IK3'(c) 1989 */
	DRIVER( pow )		/* A7008 'DG' (c) 1988 */
	DRIVER( powj )		/* A7008 'DG' (c) 1988 */
	DRIVER( searchar )	/* A8007 'BH' (c) 1989 */
	DRIVER( sercharu )	/* A8007 'BH' (c) 1989 */
	DRIVER( sercharj )	/* A8007 'BH' (c) 1989 */
	DRIVER( streetsm )	/* A8007 'S2' (c) 1989 */
	DRIVER( streets1 )	/* A7008 'S2' (c) 1989 */
	DRIVER( streetsw )	/*            (c) 1989 */
	DRIVER( streetsj )	/* A8007 'S2' (c) 1989 */
	DRIVER( prehisle )	/* A8003 'GT' (c) 1989 */
	DRIVER( prehislu )	/* A8003 'GT' (c) 1989 */
	DRIVER( gensitou )	/* A8003 'GT' (c) 1989 */
	DRIVER( mechatt )	/* A8002 'MA' (c) 1989 */
	DRIVER( mechattu )	/* A8002 'MA' (c) 1989 */
	DRIVER( bbusters )	/* A9003 'BB' (c) 1989 */

	/* Electrocoin on early SNK hardware - gambling game remove once it works */
	DRIVER( dmndrby )	/* G4001 'DD' (c) 1986 */
	DRIVER( dmndrbya )	/* G4001 'DD' (c) 1986 */

	/* Neo Geo games */
	/* the four digits number is the game ID stored at address 0x0108 of the program ROM */
	/* info on prototypes taken from http://www.members.tripod.com/fresa/proto/puzzle.htm */
 	DRIVER( neogeo )
	DRIVER( nam1975 )	/* 0001 (c) 1990 SNK */
	DRIVER( bstars )	/* 0002 (c) 1990 SNK */
	DRIVER( tpgolf )	/* 0003 (c) 1990 SNK */
	DRIVER( mahretsu )	/* 0004 (c) 1990 SNK */
	DRIVER( maglord )	/* 0005 (c) 1990 Alpha Denshi Co. */
	DRIVER( maglordh )	/* 0005 (c) 1990 Alpha Denshi Co. */
	DRIVER( ridhero )	/* 0006 (c) 1990 SNK */
	DRIVER( ridheroh )	/* 0006 (c) 1990 SNK */
	DRIVER( alpham2 )	/* 0007 (c) 1991 SNK */
				/* 0008 Sunshine (prototype) 1990 SNK */
	DRIVER( ncombat )	/* 0009 (c) 1990 Alpha Denshi Co. */
	DRIVER( ncombata )	/* 0009 (c) 1990 Alpha Denshi Co. */
	DRIVER( cyberlip )	/* 0010 (c) 1990 SNK */
	DRIVER( superspy )	/* 0011 (c) 1990 SNK */
				/* 0012 */
				/* 0013 */
	DRIVER( mutnat )	/* 0014 (c) 1992 SNK */
				/* 0015 */
	DRIVER( kotm )		/* 0016 (c) 1991 SNK */
	DRIVER( kotmh )		/* 0016 (c) 1991 SNK */
	DRIVER( sengoku )	/* 0017 (c) 1991 SNK */
	DRIVER( sengokh )	/* 0017 (c) 1991 SNK */
	DRIVER( burningf )	/* 0018 (c) 1991 SNK */
	DRIVER( burningh )	/* 0018 (c) 1991 SNK */
	DRIVER( lbowling )	/* 0019 (c) 1990 SNK */
	DRIVER( gpilots )	/* 0020 (c) 1991 SNK */
	DRIVER( joyjoy )	/* 0021 (c) 1990 SNK */
	DRIVER( bjourney )	/* 0022 (c) 1990 Alpha Denshi Co. */
	DRIVER( quizdais )	/* 0023 (c) 1991 SNK */
	DRIVER( lresort )	/* 0024 (c) 1992 SNK */
	DRIVER( eightman )	/* 0025 (c) 1991 SNK / Pallas */
				/* 0026 Fun Fun Brothers (prototype) 1991 Alpha */
	DRIVER( minasan )	/* 0027 (c) 1990 Monolith Corp. */
				/* 0028 Dunk Star (prototype) Sammy */
	DRIVER( legendos )	/* 0029 (c) 1991 SNK */
	DRIVER( 2020bb )	/* 0030 (c) 1991 SNK / Pallas */
	DRIVER( 2020bba )	/* 0030 (c) 1991 SNK / Pallas */
	DRIVER( 2020bbh )	/* 0030 (c) 1991 SNK / Pallas */
	DRIVER( socbrawl )	/* 0031 (c) 1991 SNK */
	DRIVER( roboarmy )	/* 0032 (c) 1991 SNK */
	DRIVER( roboarma )	/* 0032 (c) 1991 SNK */
	DRIVER( fatfury1 )	/* 0033 (c) 1991 SNK */
	DRIVER( fbfrenzy )	/* 0034 (c) 1992 SNK */
				/* 0035 Mystic Wand (prototype) 1991 Alpha */
	DRIVER( bakatono )	/* 0036 (c) 1991 Monolith Corp. */
	DRIVER( crsword )	/* 0037 (c) 1991 Alpha Denshi Co. */
	DRIVER( trally )	/* 0038 (c) 1991 Alpha Denshi Co. */
	DRIVER( kotm2 )		/* 0039 (c) 1992 SNK */
	DRIVER( sengoku2 )	/* 0040 (c) 1993 SNK */
	DRIVER( bstars2 )	/* 0041 (c) 1992 SNK */
	DRIVER( quizdai2 )	/* 0042 (c) 1992 SNK */
	DRIVER( 3countb )	/* 0043 (c) 1993 SNK */
	DRIVER( aof )		/* 0044 (c) 1992 SNK */
	DRIVER( samsho )	/* 0045 (c) 1993 SNK */
	DRIVER( tophuntr )	/* 0046 (c) 1994 SNK */
	DRIVER( tophunta )	/* 0046 (c) 1994 SNK */
	DRIVER( fatfury2 )	/* 0047 (c) 1992 SNK */
	DRIVER( janshin )	/* 0048 (c) 1994 Aicom */
	DRIVER( androdun )	/* 0049 (c) 1992 Visco */
	DRIVER( ncommand )	/* 0050 (c) 1992 Alpha Denshi Co. */
	DRIVER( viewpoin )	/* 0051 (c) 1992 Sammy */
	DRIVER( ssideki )	/* 0052 (c) 1992 SNK */
	DRIVER( wh1 )		/* 0053 (c) 1992 Alpha Denshi Co. */
	DRIVER( wh1h )		/* 0053 (c) 1992 Alpha Denshi Co. */
				/* 0054 Crossed Swords 2  (CD only? not confirmed, MVS might exist) */
	DRIVER( kof94 )		/* 0055 (c) 1994 SNK */
	DRIVER( aof2 )		/* 0056 (c) 1994 SNK */
	DRIVER( aof2a )		/* 0056 (c) 1994 SNK */
	DRIVER( wh2 )		/* 0057 (c) 1993 ADK */
	DRIVER( fatfursp )	/* 0058 (c) 1993 SNK */
	DRIVER( fatfursa )	/* 0058 (c) 1993 SNK */
	DRIVER( savagere )	/* 0059 (c) 1995 SNK */
	DRIVER( fightfev )	/* 0060 (c) 1994 Viccom */
	DRIVER( fightfva )	/* 0060 (c) 1994 Viccom */
	DRIVER( ssideki2 )	/* 0061 (c) 1994 SNK */
	DRIVER( spinmast )	/* 0062 (c) 1993 Data East Corporation */
	DRIVER( samsho2 )	/* 0063 (c) 1994 SNK */
	DRIVER( wh2j )		/* 0064 (c) 1994 ADK / SNK */
	DRIVER( wjammers )	/* 0065 (c) 1994 Data East Corporation */
	DRIVER( karnovr )	/* 0066 (c) 1994 Data East Corporation */
	DRIVER( gururin )	/* 0067 (c) 1994 Face */
	DRIVER( pspikes2 )	/* 0068 (c) 1994 Video System Co. */
	DRIVER( fatfury3 )	/* 0069 (c) 1995 SNK */
	DRIVER( zupapa )	/* 0070 Zupapa - released in 2001, 1994 prototype probably exists */
				/* 0071 Bang Bang Busters (prototype) 1994 Visco */
				/* 0072 Last Odyssey Pinball Fantasia (prototype) 1995 Monolith */
	DRIVER( panicbom )	/* 0073 (c) 1994 Eighting / Hudson */
	DRIVER( aodk )		/* 0074 (c) 1994 ADK / SNK */
	DRIVER( sonicwi2 )	/* 0075 (c) 1994 Video System Co. */
	DRIVER( zedblade )	/* 0076 (c) 1994 NMK */
				/* 0077 The Warlocks of the Fates (prototype) 1995 Astec */
	DRIVER( galaxyfg )	/* 0078 (c) 1995 Sunsoft */
	DRIVER( strhoop )	/* 0079 (c) 1994 Data East Corporation */
	DRIVER( quizkof )	/* 0080 (c) 1995 Saurus */
	DRIVER( ssideki3 )	/* 0081 (c) 1995 SNK */
	DRIVER( doubledr )	/* 0082 (c) 1995 Technos */
	DRIVER( pbobblen )	/* 0083 (c) 1994 Taito */
	DRIVER( pbobblna )	/* 0083 (c) 1994 Taito */
	DRIVER( kof95 )		/* 0084 (c) 1995 SNK */
	DRIVER( kof95a )	/* 0084 (c) 1995 SNK */
				/* 0085 Shinsetsu Samurai Spirits Bushidoretsuden / Samurai Shodown RPG (CD only) */
	DRIVER( tws96 )		/* 0086 (c) 1996 Tecmo */
	DRIVER( samsho3 )	/* 0087 (c) 1995 SNK */
	DRIVER( samsho3a )	/* 0087 (c) 1995 SNK */
	DRIVER( fswords )	/* Korean hack of samsho3 */
	DRIVER( stakwin )	/* 0088 (c) 1995 Saurus */
	DRIVER( pulstar )	/* 0089 (c) 1995 Aicom */
	DRIVER( whp )		/* 0090 (c) 1995 ADK / SNK */
				/* 0091 */
	DRIVER( kabukikl )	/* 0092 (c) 1995 Hudson */
	DRIVER( neobombe )	/* 0093 (c) 1997 Hudson */
	DRIVER( gowcaizr )	/* 0094 (c) 1995 Technos */
	DRIVER( rbff1 )		/* 0095 (c) 1995 SNK */
	DRIVER( aof3 )		/* 0096 (c) 1996 SNK */
	DRIVER( aof3k )		/* Censored Korean release of rbff2 */
	DRIVER( sonicwi3 )	/* 0097 (c) 1995 Video System Co. */
				/* 0098 Idol Mahjong - final romance 2 (CD only? not confirmed, MVS might exist) */
				/* 0099 Neo Pool Masters */
	DRIVER( turfmast )	/* 0200 (c) 1996 Nazca */
	DRIVER( mslug )		/* 0201 (c) 1996 Nazca */
	DRIVER( puzzledp )	/* 0202 (c) 1995 Taito (Visco license) */
	DRIVER( mosyougi )	/* 0203 (c) 1995 ADK / SNK */
				/* 0204 QP (prototype) */
				/* 0205 Neo-Geo CD Special (CD only) */
	DRIVER( marukodq )	/* 0206 (c) 1995 Takara */
	DRIVER( neomrdo )	/* 0207 (c) 1996 Visco */
	DRIVER( sdodgeb )	/* 0208 (c) 1996 Technos */
	DRIVER( goalx3 )	/* 0209 (c) 1995 Visco */
				/* 0210 Karate Ninja Sho (prototype) 1995 Yumekobo */
				/* 0211 Oshidashi Zintrick (CD only? not confirmed, MVS might exist) 1996 SNK/ADK */
	DRIVER( zintrckb )	/* 0211 hack - this is not a genuine MVS proto, its a bootleg made from the CD version */
	DRIVER( overtop )	/* 0212 (c) 1996 ADK */
	DRIVER( neodrift )	/* 0213 (c) 1996 Visco */
	DRIVER( kof96 )		/* 0214 (c) 1996 SNK */
	DRIVER( kof96h )	/* 0214 (c) 1996 SNK */
	DRIVER( ssideki4 )	/* 0215 (c) 1996 SNK */
	DRIVER( kizuna )	/* 0216 (c) 1996 SNK */
	DRIVER( ninjamas )	/* 0217 (c) 1996 ADK / SNK */
	DRIVER( ragnagrd )	/* 0218 (c) 1996 Saurus */
	DRIVER( pgoal )		/* 0219 (c) 1996 Saurus */
				/* 0220 Choutetsu Brikin'ger - iron clad (MVS existance seems to have been confirmed) */
	DRIVER( magdrop2 )	/* 0221 (c) 1996 Data East Corporation */
	DRIVER( samsho4 )	/* 0222 (c) 1996 SNK */
	DRIVER( rbffspec )	/* 0223 (c) 1996 SNK */
	DRIVER( twinspri )	/* 0224 (c) 1996 ADK */
	DRIVER( wakuwak7 )	/* 0225 (c) 1996 Sunsoft */
				/* 0226 Pair Pair Wars (prototype) 1996 Sunsoft? */
	DRIVER( stakwin2 )	/* 0227 (c) 1996 Saurus */
	DRIVER( ghostlop )	/* 0228 GhostLop (prototype) 1996? Data East */
				/* 0229 King of Fighters '96 CD Collection (CD only) */
	DRIVER( breakers )	/* 0230 (c) 1996 Visco */
	DRIVER( miexchng )	/* 0231 (c) 1997 Face */
	DRIVER( kof97 )		/* 0232 (c) 1997 SNK */
	DRIVER( kof97a )	/* 0232 (c) 1997 SNK */
	DRIVER( kof97pls )	/* bootleg of kof97 */
	DRIVER( kog )		/* bootleg of kof97 */
	DRIVER( magdrop3 )	/* 0233 (c) 1997 Data East Corporation */
	DRIVER( lastblad )	/* 0234 (c) 1997 SNK */
	DRIVER( lastblda )	/* 0234 (c) 1997 SNK */
	DRIVER( lastsold )	/* Censored Korean release of rbff2 */
	DRIVER( puzzldpr )	/* 0235 (c) 1997 Taito (Visco license) */
	DRIVER( irrmaze )	/* 0236 (c) 1997 SNK / Saurus */
	DRIVER( popbounc )	/* 0237 (c) 1997 Video System Co. */
	DRIVER( shocktro )	/* 0238 (c) 1997 Saurus */
	DRIVER( shocktra )	/* 0238 (c) 1997 Saurus */
	DRIVER( blazstar )	/* 0239 (c) 1998 Yumekobo */
	DRIVER( rbff2 )		/* 0240 (c) 1998 SNK */
	DRIVER( rbff2a )	/* 0240 (c) 1998 SNK */
	DRIVER( rbff2k )	/* Censored Korean release of rbff2 */
	DRIVER( mslug2 )	/* 0241 (c) 1998 SNK */
	DRIVER( kof98 )		/* 0242 (c) 1998 SNK */
	DRIVER( kof98k )	/* 0242 (c) 1998 SNK */
	DRIVER( kof98n )	/* 0242 (c) 1998 SNK */
	DRIVER( lastbld2 )	/* 0243 (c) 1998 SNK */
	DRIVER( neocup98 )	/* 0244 (c) 1998 SNK */
	DRIVER( breakrev )	/* 0245 (c) 1998 Visco */
	DRIVER( shocktr2 )	/* 0246 (c) 1998 Saurus */
	DRIVER( lans2004 )	/* bootleg of shocktr2 */
	DRIVER( flipshot )	/* 0247 (c) 1998 Visco */
	DRIVER( pbobbl2n )	/* 0248 (c) 1999 Taito (SNK license) */
	DRIVER( ctomaday )	/* 0249 (c) 1999 Visco */
	DRIVER( mslugx )	/* 0250 (c) 1999 SNK */
	DRIVER( kof99 )		/* 0251 (c) 1999 SNK */
	DRIVER( kof99a )	/* 0251 (c) 1999 SNK */
	DRIVER( kof99e )	/* 0251 (c) 1999 SNK */
	DRIVER( kof99n )	/* 0251 (c) 1999 SNK */
	DRIVER( kof99p )	/* 0251 (c) 1999 SNK */
	DRIVER( ganryu )	/* 0252 (c) 1999 Visco */
	DRIVER( garou )		/* 0253 (c) 1999 SNK */
	DRIVER( garouo )	/* 0253 (c) 1999 SNK */
	DRIVER( garoup )	/* 0253 (c) 1999 SNK */
	DRIVER( s1945p )	/* 0254 (c) 1999 Psikyo */
	DRIVER( preisle2 )	/* 0255 (c) 1999 Yumekobo */
	DRIVER( mslug3 )	/* 0256 (c) 2000 SNK */
	DRIVER( mslug3n )	/* 0256 (c) 2000 SNK */
	DRIVER( mslug3b6 )	/* bootleg */
	DRIVER( kof2000 )	/* 0257 (c) 2000 SNK */
	DRIVER( kof2000n )	/* 0257 (c) 2000 SNK */
				/* 0258 SNK vs. Capcom? */
	DRIVER( bangbead )	/* 0259 (c) 2000 Visco */
	DRIVER( nitd )		/* 0260 (c) 2000 Eleven / Gavaking */
	DRIVER( sengoku3 )	/* 0261 (c) 2001 SNK */
	DRIVER( kof2001 )	/* 0262 (c) 2001 Eolith / SNK */
	DRIVER( kof2001h )	/* 0262 (c) 2001 Eolith / SNK */
	DRIVER( cthd2003 )	/* bootleg of kof2001*/
	DRIVER( ct2k3sp )	/* bootleg of kof2001 */
	DRIVER( mslug4 )	/* 0263 (c) 2002 Mega Enterprise */
	DRIVER( ms4plus )	/* 0263 bootleg */
	DRIVER( rotd )		/* 0264 (c) 2002 Evoga */
	DRIVER( kof2002 )	/* 0265 (c) 2002 Eolith / Playmore */
	DRIVER( kf2k2pls )	/* bootleg */
	DRIVER( kf2k2pla )	/* bootleg */
	DRIVER( kf2k2mp )	/* bootleg */
	DRIVER( kf2k2mp2 )	/* bootleg */
	DRIVER( kof10th )	/* bootleg of kof2002 */
	DRIVER( kf2k5uni )	/* bootleg of kof2002 */
	DRIVER( kf10thep )	/* bootleg of kof2002 */
	DRIVER( kof2k4se )	/* bootleg of kof2002 */
	DRIVER( matrim )	/* 0266 (c) 2002 Atlus */
	DRIVER( pnyaa )		/* 0267 (c) 2003 Aiky / Taito */
	DRIVER( ms5pcb )	/* 0268 (c) 2003 Playmore */
	DRIVER( mslug5 )	/* 0268 (c) 2003 Playmore */
	DRIVER( ms5plus )	/* bootleg */
	DRIVER( svcpcb )	/* 0269 (c) 2003 Playmore / Capcom - JAMMA PCB */
	DRIVER( svcpcba )	/* 0269 (c) 2003 Playmore / Capcom - JAMMA PCB */
	DRIVER( svc )		/* 0269 (c) 2003 Playmore / Capcom */
	DRIVER( svcboot )	/* bootleg */
	DRIVER( svcplus )	/* bootleg */
	DRIVER( svcplusa )	/* bootleg */
	DRIVER( svcsplus )	/* bootleg */
	DRIVER( samsho5 )	/* 0270 (c) 2003 Playmore */
	DRIVER( samsho5h )	/* 0270 (c) 2003 Playmore */
	DRIVER( samsho5b )	/* bootleg */
	DRIVER( kf2k3pcb )	/* 0271 (c) 2003 Playmore - JAMMA PCB */
	DRIVER( kof2003 )	/* 0271 (c) 2003 Playmore */
	DRIVER( kf2k3bl )	/* bootleg */
	DRIVER( kf2k3bla )	/* bootleg */
	DRIVER( kf2k3pl )	/* bootleg */
	DRIVER( kf2k3upl )	/* bootleg */
	DRIVER( samsh5sp )	/* 0272 (c) 2003 Playmore */
	DRIVER( samsh5sh )	/* 0272 (c) 2003 Playmore */
	DRIVER( samsh5sn )	/* 0272 (c) 2003 Playmore */


	/* The BreezaSoft games don't have proper ID codes */
	DRIVER( jockeygp )
	DRIVER( vliner )
	DRIVER( vlinero )

	/* Hyper NeoGeo 64 uses a 3 digit rom code? */

 	DRIVER( hng64 )
	DRIVER( roadedge ) 	/* 001? Roads Edge / Round Trip RV */
				/* 002? Samurai Shodown 64 / Samurai Spirits 64 */
				/* 003? Xtreme Rally / Off Beat Racer! */
				/* 004? Beast Busters: Second Nightmare */
	DRIVER( sams64_2 )	/* 005? Samurai Shodown: Warrior's Rage / Samurai Spirits: Asura Zanmaden */
	DRIVER( fatfurwa )	/* 006? Fatal Fury: Wild Ambition / Garou Densetsu: Wild Ambition */
	DRIVER( buriki )	/* 007 (c) 1999 SNK */

	/* Alpha Denshi games */
	DRIVER( shougi )
	DRIVER( shougi2 )
	DRIVER( champbas )	/* (c) 1983 Sega */
	DRIVER( champbbj )	/* (c) 1983 Alpha Denshi Co. */
	DRIVER( champbja )	/* (c) 1983 Alpha Denshi Co. */
	DRIVER( champbb2 )	/* (c) 1983 Sega */
	DRIVER( champb2a )	/* (c) 1983 ? */
	DRIVER( exctsccr )	/* (c) 1983 Alpha Denshi Co. */
	DRIVER( exctscca )	/* (c) 1983 Alpha Denshi Co. */
	DRIVER( exctsccb )	/* bootleg */
	DRIVER( exctscc2 )	/* (c) 1984 Alpha Denshi Co. */
	DRIVER( talbot )	/* Volt Electronics */
	DRIVER( equites )	/* (c) 1984 Alpha Denshi Co. */
	DRIVER( equitess )	/* (c) 1984 Alpha Denshi Co./Sega */
	DRIVER( gekisou )	/* (c) 1985 Eastern */
	DRIVER( bullfgtr )	/* (c) 1984 Alpha Denshi Co. */
	DRIVER( bullfgts )	/* (c) 1984 Alpha Denshi Co./Sega */
	DRIVER( kouyakyu )	/* (c) 1985 Alpha Denshi Co. */
	DRIVER( splndrbt )	/* (c) 1985 Alpha Denshi Co. */
	DRIVER( hvoltage )	/* (c) 1985 Alpha Denshi Co. */

	/* SNK / Alpha 68K games */
	DRIVER( sstingry )	/* (c) 1986 Alpha Denshi Co. */
	DRIVER( kyros )		/* (c) 1987 World Games */
	DRIVER( kyrosj )	/* (c) 1986 Alpha Denshi Co. */
	DRIVER( jongbou )	/* (c) 1987 SNK */
	DRIVER( paddlema )	/* Alpha-68K96I  'PM' (c) 1988 SNK */
	DRIVER( timesold )	/* Alpha-68K96II 'BT' (c) 1987 SNK / Romstar */
	DRIVER( timesol1 )	/* Alpha-68K96II 'BT' (c) 1987 */
	DRIVER( btlfield )	/* Alpha-68K96II 'BT' (c) 1987 */
	DRIVER( btlfildb )	/* bootleg */
	DRIVER( skysoldr )	/* Alpha-68K96II 'SS' (c) 1988 SNK (Romstar with dip switch) */
	DRIVER( goldmedl )	/* Alpha-68K96II 'GM' (c) 1988 SNK */
	DRIVER( goldmeda )	/* Alpha-68K96II 'GM' (c) 1988 SNK */
	DRIVER( goldmedb )	/* Alpha-68K96II bootleg */
	DRIVER( skyadvnt )	/* Alpha-68K96V  'SA' (c) 1989 Alpha Denshi Co. */
	DRIVER( skyadvnu )	/* Alpha-68K96V  'SA' (c) 1989 SNK of America licensed from Alpha */
	DRIVER( skyadvnj )	/* Alpha-68K96V  'SA' (c) 1989 Alpha Denshi Co. */
	DRIVER( gangwars )	/* Alpha-68K96V       (c) 1989 Alpha Denshi Co. */
	DRIVER( gangwarb )	/* Alpha-68K96V bootleg */
	DRIVER( sbasebal )	/* Alpha-68K96V       (c) 1989 SNK of America licensed from Alpha */
	DRIVER( tnexspce )	/* A8003 'NS' (c) 1989 */

	DRIVER( meijinsn )

	/* Technos games */
	DRIVER( scregg )	/* TA-0001 (c) 1983 */
	DRIVER( eggs )		/* TA-0002 (c) 1983 Universal USA */
	DRIVER( dommy )		/* TA-00?? (c) */
	DRIVER( bigprowr )	/* TA-0007 (c) 1983 */
	DRIVER( tagteam )	/* TA-0007 (c) 1983 + Data East license */
	DRIVER( ssozumo )	/* TA-0008 (c) 1984 */
	DRIVER( mystston )	/* TA-0010 (c) 1984 */
	DRIVER( myststno )	/* TA-0010 (c) 1984 */
	DRIVER( dogfgt )	/* TA-0011 (c) 1984 */
	DRIVER( dogfgtj )	/* TA-0011 (c) 1984 */
	DRIVER( bogeyman )	/* -0204-0 (Data East part number) (c) [1985?] */
	DRIVER( matmania )	/* TA-0015 (c) 1985 + Taito America license */
	DRIVER( excthour )	/* TA-0015 (c) 1985 + Taito license */
	DRIVER( maniach )	/* TA-0017 (c) 1986 + Taito America license */
	DRIVER( maniach2 )	/* TA-0017 (c) 1986 + Taito America license */
	DRIVER( renegade )	/* TA-0018 (c) 1986 + Taito America license */
	DRIVER( kuniokun )	/* TA-0018 (c) 1986 */
	DRIVER( kuniokub )	/* bootleg */
	DRIVER( xsleena )	/* TA-0019 (c) 1986 */
	DRIVER( xsleenab )	/* bootleg */
	DRIVER( solarwar )	/* TA-0019 (c) 1986 Taito + Memetron license */
	DRIVER( battlane )	/* -0215, -0216 (Data East part number) (c) 1986 + Taito license */
	DRIVER( battlan2 )	/* -0215, -0216 (Data East part number) (c) 1986 + Taito license */
	DRIVER( battlan3 )	/* -0215, -0216 (Data East part number) (c) 1986 + Taito license */
	DRIVER( ddragon )	/* TA-0021 (c) 1987 */
	DRIVER( ddragonu )	/* TA-0021 (c) 1987 Taito America */
	DRIVER( ddragonw )	/* TA-0021 (c) 1987 Taito */
	DRIVER( ddragnw1 )	/* TA-0021 (c) 1987 Taito */
	DRIVER( ddragonb )	/* bootleg */
	DRIVER( spdodgeb )	/* TA-0022 (c) 1987 */
	DRIVER( nkdodgeb )	/* TA-0022 (c) 1987 (Japan) */
	DRIVER( chinagat )	/* TA-0023 (c) 1988 Taito + Romstar license (US) */
	DRIVER( saiyugou )	/* TA-0023 (c) 1988 (Japan) */
	DRIVER( saiyugb1 )	/* bootleg */
	DRIVER( saiyugb2 )	/* bootleg */
	DRIVER( wwfsstar )	/* TA-0024 (c) 1989 (Europe) */
	DRIVER( wwfsstaa )	/* TA-0024 (c) 1989 (US, newer) */
	DRIVER( wwfsstau )	/* TA-0024 (c) 1989 (US) */
	DRIVER( wwfsstaj )	/* TA-0024 (c) 1989 (Japan) */
	DRIVER( vball )		/* TA-0025 (c) 1988 */
	DRIVER( vball2pj )	/* TA-0025 (c) 1988 (Japan) */
	DRIVER( ddragon2 )	/* TA-0026 (c) 1988 (World) */
	DRIVER( ddragn2u )	/* TA-0026 (c) 1988 (US) */
	DRIVER( toffy )		/* (c) 1993 Midas */
	DRIVER( stoffy )	/* (c) 1994 Midas + Unico */
	DRIVER( ddungeon )	/* Game Room */
	DRIVER( darktowr )	/* Game Room */
	DRIVER( tstrike )	/* Game Room */
	DRIVER( tstrikea )	/* Game Room */
	DRIVER( ctribe )	/* TA-0028 (c) 1990 (US) */
	DRIVER( ctribeb )	/* bootleg */
	DRIVER( blockout )	/* TA-0029 (c) 1989 + California Dreams */
	DRIVER( blckout2 )	/* TA-0029 (c) 1989 + California Dreams */
	DRIVER( blckoutj )	/* TA-0029 (c) 1989 + California Dreams (Japan) */
	DRIVER( agress )	/* TA-0029 (c) 1991 Palco */
	DRIVER( agressb )	/* TA-0029 (c) 1991 Palco */
	DRIVER( ddragon3 )	/* TA-0030 (c) 1990 */
	DRIVER( ddrago3b )	/* bootleg */
	DRIVER( wwfwfest )	/* TA-0031 (c) 1991 (US) */
	DRIVER( wwfwfsta )	/* TA-0031 (c) 1991 + Tecmo license (US) */
	DRIVER( wwfwfstb )	/* bootleg */
	DRIVER( wwfwfstj )	/* TA-0031 (c) 1991 (Japan) */
	DRIVER( shadfrce )	/* TA-0032 (c) 1993 (US) */

	/* Stern "Berzerk hardware" games */
	DRIVER( berzerk )	/* (c) 1980 */
	DRIVER( berzerk1 )	/* (c) 1980 */
	DRIVER( frenzy )	/* (c) 1982 */

	/* Stern 'Mazer Blazer hardware' games */
	DRIVER( mazerbla )
	DRIVER( mzrblzra )
	DRIVER( greatgun )

	/* Other Stern */
	DRIVER( supdrapo )

	/* GamePlan games */
	DRIVER( toratora )	/* (c) 1980 Game Plan */
	DRIVER( megatack )	/* (c) 1980 Centuri */
	DRIVER( killcom )	/* (c) 1980 Centuri */
	DRIVER( challeng )	/* (c) 1981 Centuri */
	DRIVER( kaos )		/* (c) 1981 */

	/* Zaccaria games */
	DRIVER( sia2650 )	/* (c) 1978 */
	DRIVER( tinv2650 )	/* (c) 1978 */
	DRIVER( dodgem )	/* (c) 1979 */
	DRIVER( galaxia )	/* (c) 1979 */
	DRIVER( laserbat )	/* (c) 1981 */
	DRIVER( lazarian )	/* (c) 1981 */
	DRIVER( catnmous )	/* (c) 1982 */
	DRIVER( catmousa )	/* (c) 1982 */
	DRIVER( monymony )	/* (c) 1983 */
	DRIVER( jackrabt )	/* (c) 1984 */
	DRIVER( jackrab2 )	/* (c) 1984 */
	DRIVER( jackrabs )	/* (c) 1984 */

	/* UPL games */
	DRIVER( mouser )	/* UPL-83001 (c) 1983 */
	DRIVER( mouserc )	/* UPL-83001 (c) 1983 */
	DRIVER( nova2001 )	/* UPL-83005 (c) 1983 */
	DRIVER( nov2001u )	/* UPL-83005 (c) [1983] + Universal license */
	DRIVER( ninjakun )	/* UPL-84003 (c) 1984 Taito Corporation */
	DRIVER( raiders5 )	/* UPL-85004 (c) 1985 */
	DRIVER( raidrs5t )
	DRIVER( pkunwar )	/* UPL-????? [1985?] */
	DRIVER( pkunwarj )	/* UPL-????? [1985?] */
	DRIVER( xxmissio )	/* UPL-86001 [1986] */
	DRIVER( ninjakd2 )	/* UPL-????? (c) 1987 */
	DRIVER( ninjak2a )	/* UPL-????? (c) 1987 */
	DRIVER( ninjak2b )	/* UPL-????? (c) 1987 */
	DRIVER( rdaction )	/* UPL-87003?(c) 1987 + World Games license */
	DRIVER( mnight )	/* UPL-????? (c) 1987 distributed by Kawakus */
	DRIVER( arkarea )	/* UPL-87007 (c) [1988?] */
	DRIVER( robokid )	/* UPL-88013 (c) 1988 */
	DRIVER( robokidj )	/* UPL-88013 (c) 1988 */
	DRIVER( robokdj2 )	/* UPL-88013 (c) 1988 */
	DRIVER( omegaf )	/* UPL-89016 (c) 1989 */
	DRIVER( omegafs )	/* UPL-89016 (c) 1989 */

	/* UPL/NMK/Banpresto games */
	DRIVER( urashima )	/* UPL-89052 */
	DRIVER( tharrier )	/* UPL-89053 (c) 1989 UPL + American Sammy license */
	DRIVER( tharierj )	/* UPL-89053 (c) 1989 UPL (Japan) */
	DRIVER( mustang )	/* UPL-90058 (c) 1990 UPL */
	DRIVER( mustangs )	/* UPL-90058 (c) 1990 UPL + Seoul Trading */
	DRIVER( mustangb )	/* bootleg */
	DRIVER( bioship )	/* UPL-90062 (c) 1990 UPL + American Sammy license */
	DRIVER( vandyke )	/* UPL-90064 (c) UPL */
	DRIVER( vandyjal )	/* UPL-90064 (c) Jaleco */
	DRIVER( vandyja2 )	/* UPL-90064 (c) Jaleco */
	DRIVER( blkheart )	/* UPL-91069 */
	DRIVER( blkhearj )	/* UPL-91069 */
	DRIVER( acrobatm )	/* UPL-91073 (c) 1991 UPL + Taito license */
	DRIVER( strahl )	/* UPL-91074 (c) 1992 UPL (Japan) */
	DRIVER( strahla )	/* UPL-91074 (c) 1992 UPL (Japan) */
	DRIVER( bjtwin )	/* UPL-93087 (c) 1993 NMK */
	DRIVER( tdragon2 )	/* UPL-93091 (c) 1993 NMK */
	DRIVER( bigbang )	/* UPL-93091 (c) 1993 NMK */
	DRIVER( tdragon )	/* (c) 1991 NMK / Tecmo */
	DRIVER( tdragonb )	/* bootleg */
	DRIVER( hachamf )	/* (c) 1991 NMK */
	DRIVER( macross )	/* (c) 1992 Banpresto */
	DRIVER( riot )		/* (c) 1992 NMK */
	DRIVER( gunnail )	/* (c) 1993 NMK / Tecmo */
	DRIVER( macross2 )	/* (c) 1993 Banpresto */
	DRIVER( sabotenb )	/* (c) 1992 NMK / Tecmo */
	DRIVER( sabotnba )	/* (c) 1992 NMK / Tecmo */
	DRIVER( nouryoku )	/* (c) 1995 Tecmo */
	DRIVER( manybloc )	/* (c) 1991 Bee-Oh */
	DRIVER( ssmissin )	/* (c) 1992 Comad */
	DRIVER( airattck )	/* (c) 1996 Comad */
	DRIVER( raphero )	/* (c) 1994 Media Trading Corp */

	/* Jaleco Mahjong Games, Similar Hardware to the NMK ones above? */
	DRIVER( daireika )
	DRIVER( mjzoomin )
	DRIVER( kakumei )
	DRIVER( kakumei2 )
	DRIVER( suchipi )

	/* Based on nmk16? */
	DRIVER( quizpani )
	DRIVER( acommand )

	/* don't know what hardare Banpresto used for these games */
	DRIVER( macrossp )	/* (c) 1996 Banpresto */
	DRIVER( quizmoon )	/* (c) 1997 Banpresto */

	/* Face/NMK games */
	DRIVER( gakupara )	/* (c) 1991 NMK */
	DRIVER( quizdna )	/* (c) 1992 Face */
	DRIVER( gekiretu )	/* (c) 1992 Face */

	/* Williams/Midway games */
	DRIVER( narc )		/* (c) 1988 Williams */
	DRIVER( narc3 )		/* (c) 1988 Williams */
	DRIVER( trog )		/* (c) 1990 Midway */
	DRIVER( trog3 )		/* (c) 1990 Midway */
	DRIVER( trogpa6 )	/* (c) 1990 Midway */
	DRIVER( trogp )		/* (c) 1990 Midway */
	DRIVER( smashtv )	/* (c) 1990 Williams */
	DRIVER( smashtv6 )	/* (c) 1990 Williams */
	DRIVER( smashtv5 )	/* (c) 1990 Williams */
	DRIVER( smashtv4 )	/* (c) 1990 Williams */
	DRIVER( hiimpact )	/* (c) 1990 Williams */
	DRIVER( hiimpac3 )	/* (c) 1990 Williams */
	DRIVER( hiimpacp )	/* (c) 1990 Williams */
	DRIVER( shimpact )	/* (c) 1991 Midway */
	DRIVER( shimpacp )	/* (c) 1991 Midway */
	DRIVER( strkforc )	/* (c) 1991 Midway */
	DRIVER( mk )		/* (c) 1992 Midway */
	DRIVER( mkr4 )		/* (c) 1992 Midway */
	DRIVER( mkprot9 )	/* (c) 1992 Midway */
	DRIVER( mkla1 )		/* (c) 1992 Midway */
	DRIVER( mkla2 )		/* (c) 1992 Midway */
	DRIVER( mkla3 )		/* (c) 1992 Midway */
	DRIVER( mkla4 )		/* (c) 1992 Midway */
	DRIVER( mkyawdim )
	DRIVER( term2 )		/* (c) 1992 Midway */
	DRIVER( term2la2 )	/* (c) 1992 Midway */
	DRIVER( term2la1 )	/* (c) 1992 Midway */
	DRIVER( totcarn )	/* (c) 1992 Midway */
	DRIVER( totcarnp )	/* (c) 1992 Midway */
	DRIVER( mk2 )		/* (c) 1993 Midway */
	DRIVER( mk2r32 )	/* (c) 1993 Midway */
	DRIVER( mk2r21 )	/* (c) 1993 Midway */
	DRIVER( mk2r14 )	/* (c) 1993 Midway */
	DRIVER( mk2r42 )	/* hack */
	DRIVER( mk2r91 )	/* hack */
	DRIVER( mk2chal )	/* hack */
	DRIVER( jdreddp )	/* (c) 1993 Midway */
	DRIVER( nbajam )	/* (c) 1993 Midway */
	DRIVER( nbajamr2 )	/* (c) 1993 Midway */
	DRIVER( nbajamte )	/* (c) 1994 Midway */
	DRIVER( nbajamt1 )	/* (c) 1994 Midway */
	DRIVER( nbajamt2 )	/* (c) 1994 Midway */
	DRIVER( nbajamt3 )	/* (c) 1994 Midway */
	DRIVER( revx )		/* (c) 1994 Midway */
	DRIVER( mk3 )		/* (c) 1994 Midway */
	DRIVER( mk3r20 )	/* (c) 1994 Midway */
	DRIVER( mk3r10 )	/* (c) 1994 Midway */
	DRIVER( umk3 )		/* (c) 1994 Midway */
	DRIVER( umk3r11 )	/* (c) 1994 Midway */
	DRIVER( umk3r10 )	/* (c) 1994 Midway */
	DRIVER( wwfmania )	/* (c) 1995 Midway */
	DRIVER( openice )	/* (c) 1995 Midway */
	DRIVER( nbahangt )	/* (c) 1996 Midway */
	DRIVER( nbamht )	/* (c) 1996 Midway */
	DRIVER( nbamht1 )	/* (c) 1996 Midway */
	DRIVER( rmpgwt )	/* (c) 1997 Midway */
	DRIVER( rmpgwt11 )	/* (c) 1997 Midway */
	DRIVER( crusnusa )	/* (c) 1994 Midway */
	DRIVER( crusnu40 )	/* (c) 1994 Midway */
	DRIVER( crusnu21 )	/* (c) 1994 Midway */
	DRIVER( crusnwld )	/* (c) 1996 Midway */
	DRIVER( crusnw20 )	/* (c) 1996 Midway */
	DRIVER( crusnw13 )	/* (c) 1996 Midway */
	DRIVER( offroadc )	/* (c) 1997 Midway */
	DRIVER( wargods )	/* (c) 1996 Midway */
	DRIVER( mk4 )		/* (c) 1997 Midway */
	DRIVER( invasn )	/* (c) 1999 Midway */
	DRIVER( crusnexo )	/* (c) 1999 Midway */
	DRIVER( thegrid )	/* (c) 2001 Midway */

	/* Midway / Atari  Voodoo based Hardware (Seattle, Flagstaff) */
	DRIVER( wg3dh )		/* (c) 1996 Atari Games */
	DRIVER( mace )		/* (c) 1996 Atari Games */
	DRIVER( macea )		/* (c) 1996 Atari Games */
	DRIVER( sfrush )	/* (c) 1996 Atari Games */
	DRIVER( sfrushrk )	/* (c) 1996 Atari Games */
	DRIVER( calspeed )	/* (c) 1996 Atari Games */
	DRIVER( calspeda )	/* (c) 1996 Atari Games */
	DRIVER( vaportrx )	/* (c) 1998 Atari Games */
	DRIVER( vaportrp )	/* (c) 1998 Atari Games */
	DRIVER( carnevil )	/* (c) 1998 Midway Games */
	DRIVER( biofreak )	/* (c) 1997 Midway Games (prototype) */
	DRIVER( blitz )		/* (c) 1997 Midway Games */
	DRIVER( blitz11 )	/* (c) 1997 Midway Games */
	DRIVER( blitz99 )	/* (c) 1998 Midway Games */
	DRIVER( blitz2k )	/* (c) 1999 Midway Games */
	DRIVER( hyprdriv )	/* (c) 1998 Midway Games */

	/* Midway / Atari  Voodoo 2 & 3 - based Hardware (Vegas, Denver, Durango) */
	DRIVER( gauntleg )	/* (c) 1998 Atari Games */
	DRIVER( gauntl12 )	/* (c) 1998 Atari Games */
	DRIVER( tenthdeg )	/* (c) 1998 Atari Games (prototype) */
	DRIVER( roadburn )	/* (c) 1999 Atari Games */
	DRIVER( sf2049 )	/* (c) 1999 Atari Games */
	DRIVER( sf2049se )	/* (c) 1999 Atari Games */
	DRIVER( sf2049te )	/* (c) 1999 Atari Games */
	DRIVER( warfa )		/* (c) 1999 Atari Games */
	DRIVER( nbashowt )	/* (c) 1998 Midway Games */
	DRIVER( nbanfl )	/* (c) 1999 Midway Games */
	DRIVER( gauntdl )	/* (c) 1999 Midway Games */
	DRIVER( gauntd24 )	/* (c) 1999 Midway Games */
	DRIVER( cartfury )	/* (c) 2000 Midway Games */

	/* Cinematronics raster games */
	DRIVER( embargo )
	DRIVER( tripool )	/* (c) 1981 Noma (Casino Tech license) */
	DRIVER( tripoola )	/* (c) 1981 Noma (Costal Games license) */
	DRIVER( joinem )	/* (c) 1986 Global */
	DRIVER( striv )		/* (c) 1985 Hara Industries */
	DRIVER( jack )		/* (c) 1982 Cinematronics */
	DRIVER( jack2 )		/* (c) 1982 Cinematronics */
	DRIVER( jack3 )		/* (c) 1982 Cinematronics */
	DRIVER( treahunt )	/* (c) 1982 Hara Ind. */
	DRIVER( zzyzzyxx )	/* (c) 1982 Cinematronics + Advanced Microcomputer Systems */
	DRIVER( zzyzzyx2 )	/* (c) 1982 Cinematronics + Advanced Microcomputer Systems */
	DRIVER( brix )		/* (c) 1982 Cinematronics + Advanced Microcomputer Systems */
	DRIVER( freeze )	/* Cinematronics */
	DRIVER( sucasino )	/* (c) 1982 Data Amusement */

	/* Cinematronics vector games */
	DRIVER( spacewar )
	DRIVER( barrier )
	DRIVER( starcas )	/* (c) 1980 */
	DRIVER( starcas1 )	/* (c) 1980 */
	DRIVER( starcasp )
	DRIVER( starcase )
	DRIVER( stellcas )
	DRIVER( spaceftr )
	DRIVER( tailg )
	DRIVER( ripoff )
	DRIVER( armora )
	DRIVER( armorap )
	DRIVER( armorar )
	DRIVER( wotw )
	DRIVER( wotwc )
	DRIVER( warrior )
	DRIVER( starhawk )
	DRIVER( solarq )	/* (c) 1981 */
	DRIVER( boxingb )	/* (c) 1981 */
	DRIVER( speedfrk )
	DRIVER( sundance )
	DRIVER( demon )		/* (c) 1982 Rock-ola */
	DRIVER( qb3 )
	/* this one uses 68000+Z80 instead of the Cinematronics CPU */
	DRIVER( cchasm )
	DRIVER( cchasm1 )	/* (c) 1983 Cinematronics / GCE */

	/* "The Pit hardware" games */
	DRIVER( roundup )	/* (c) 1981 Amenip/Centuri */
	DRIVER( fitter )	/* (c) 1981 Taito */
	DRIVER( thepit )	/* (c) 1982 Taito */
	DRIVER( thepitc )	/* (c) 1982 Centuri */
	DRIVER( thepitb )	/* bootleg */
	DRIVER( dockman )	/* (c) 1982 Taito Corp. */
	DRIVER( portman )	/* (c) 1982 Nova Games Ltd. */
	DRIVER( funnymou )	/* (c) 1982 Chuo Co. Ltd */
	DRIVER( suprmous )	/* (c) 1982 Taito */
	DRIVER( machomou )	/* (c) 1982 Techstar */
	DRIVER( intrepid )	/* (c) 1983 Nova Games Ltd. */
	DRIVER( intrepi2 )	/* (c) 1983 Nova Games Ltd. */
	DRIVER( zaryavos )	/* (c) 1983 Nova Games of Canada (prototype) */
	DRIVER( timelimt )	/* (c) 1983 Chuo Co. Ltd */
	DRIVER( progress )	/* (c) 1984 Chuo Co. Ltd */
	DRIVER( rtriv )		/* (c) 198? Romar */

	/* Valadon Automation games */
	DRIVER( bagman )	/* (c) 1982 */
	DRIVER( bagnard )	/* (c) 1982 */
	DRIVER( bagnarda )	/* (c) 1982 */
	DRIVER( bagmans )	/* (c) 1982 + Stern license */
	DRIVER( bagmans2 )	/* (c) 1982 + Stern license */
	DRIVER( sbagman )	/* (c) 1984 */
	DRIVER( sbagmans )	/* (c) 1984 + Stern license */
	DRIVER( pickin )	/* (c) 1983 */
	DRIVER( botanic )	/* (c) 1985 */
	DRIVER( tankbust )	/* (c) 1985 */

	/* Seibu Denshi / Seibu Kaihatsu games */
	DRIVER( stinger )	/* (c) 1983 Seibu Denshi */
	DRIVER( stinger2 )	/* (c) 1983 Seibu Denshi */
	DRIVER( scion )		/* (c) 1984 Seibu Denshi */
	DRIVER( scionc )	/* (c) 1984 Seibu Denshi + Cinematronics license */
	DRIVER( kungfut )	/* (c) 1984 Seibu Kaihatsu */
	DRIVER( kungfuta )	/* (c) 1984 Seibu Kaihatsu */
	DRIVER( wiz )		/* (c) 1985 Seibu Kaihatsu */
	DRIVER( wizt )		/* (c) 1985 Taito Corporation */
	DRIVER( kncljoe )	/* (c) 1985 Taito Corporation */
	DRIVER( kncljoea )	/* (c) 1985 Taito Corporation */
	DRIVER( bcrusher )	/* bootleg */
	DRIVER( empcity )	/* (c) 1986 Seibu Kaihatsu (bootleg?) */
	DRIVER( empcityu )	/* (c) 1986 Taito (Romstar License) */
	DRIVER( empcityj )	/* (c) 1986 Taito Corporation (Japan) */
	DRIVER( stfight )	/* (c) 1986 Seibu Kaihatsu (Germany) (bootleg?) */
	DRIVER( stfighta )	/* (c) 1986 Seibu Kaihatsu (bootleg?) */
	DRIVER( darkmist )	/* (c) 1986 Taito */
	DRIVER( mustache )	/* (c) 1987 March */
	DRIVER( cshooter )	/* (c) 1987 Taito */
	DRIVER( cshootre )	/* (c) 1987 JKH (bootleg) */
	DRIVER( airraid )	/* (c) 1987 Seibu Kaihatsu */
	DRIVER( panicr )	/* Seibu / Taito */
	DRIVER( deadang )	/* (c) 1988 Seibu Kaihatsu */
	DRIVER( ghunter )	/* (c) 1988 Seibu Kaihatsu + Segasa/Sonic license */
	DRIVER( dynduke )	/* (c) 1989 Seibu Kaihatsu + Fabtek license */
	DRIVER( dyndukf )	/* (c) 1989 Seibu Kaihatsu + Fabtek license */
	DRIVER( dbldyn )	/* (c) 1989 Seibu Kaihatsu + Fabtek license */
	DRIVER( dbldynf )	/* (c) 1989 Seibu Kaihatsu + Fabtek license */
	DRIVER( raiden )	/* (c) 1990 Seibu Kaihatsu */
	DRIVER( raidena )	/* (c) 1990 Seibu Kaihatsu */
	DRIVER( raidenk )	/* (c) 1990 Seibu Kaihatsu + IBL Corporation license */
	DRIVER( raident )	/* (c) 1990 Seibu Kaihatsu + Liang HWA Electronics license */
	DRIVER( sdgndmps )	/* (c) Banpresto / Bandai (Japan) */
	DRIVER( dcon )		/* (c) 1992 Success */
	DRIVER( sengokmj )	/* (c) 1991 Sigma */


	DRIVER( raiden2 )
	DRIVER( raiden2a )
	DRIVER( raiden2b )
	DRIVER( raiden2c )
	DRIVER( raiden2e )
	DRIVER( raidndx )
	DRIVER( raidndxa )
	DRIVER( raidndxb )
	DRIVER( raidndxj )
	DRIVER( raidndxm )       /* (c) 1994 Seibu Kaihatsu + Metrotainment license */
	DRIVER( zeroteam )
	DRIVER( zeroteaa )
	DRIVER( zeroteab )
	DRIVER( nzerotea )

	/* Seibu SPI System games */

	DRIVER( senkyu )	/* (c) 1995 Seibu Kaihatsu */
	DRIVER( senkyua )	/* (c) 1995 Seibu Kaihatsu */
	DRIVER( batlball )	/* (c) 1995 Seibu Kaihatsu (Tuning License) */
	DRIVER( batlbala )	/* (c) 1995 Seibu Kaihatsu (Metrotainment License) */

	DRIVER( viprp1 )	/* (c) 1995 Seibu Kaihatsu */
	DRIVER( viprp1s )	/* (c) 1995 Seibu Kaihatsu */
	DRIVER( viprp1o )	/* (c) 1995 Seibu Kaihatsu */
	DRIVER( viprp1ot )	/* (c) 1995 Seibu Kaihatsu (Tuning License) */

	DRIVER( ejanhs )	/* (c) 1996 Seibu Kaihatsu */

	DRIVER( rdft )		/* (c) 1996 Seibu Kaihatsu */
	DRIVER( rdftu )		/* (c) 1996 Seibu Kaihatsu (Fabtek license) */
	DRIVER( rdftau )	/* (c) 1996 Seibu Kaihatsu */
	DRIVER( rdftj )		/* (c) 1996 Seibu Kaihatsu */
	DRIVER( rdftdi )	/* (c) 1996 Seibu Kaihatsu (Dream Island license) */

	DRIVER( rdft2 )		/* (c) 1997 Seibu Kaihatsu (Tuning License) */
	DRIVER( rdft2a )	/* (c) 1997 Seibu Kaihatsu (Metrotainment license) */
	DRIVER( rdft2a2 )	/* (c) 1997 Seibu Kaihatsu (Dream Island license) */
	DRIVER( rdft2j )	/* (c) 1997 Seibu Kaihatsu */
	DRIVER( rdft2us )	/* (c) 1997 Seibu Kaihatsu (Fabtek license) */
	DRIVER( rdft22kc )	/* (c) 2000 Seibu Kaihatsu */

	DRIVER( rfjet )		/* (c) 1998 Seibu Kaihatsu (Tuning License) */
	DRIVER( rfjetu )	/* (c) 1998 Seibu Kaihatsu (Fabtek license) */
	DRIVER( rfjeta )	/* (c) 1998 Seibu Kaihatsu (Dream Island license) */
	DRIVER( rfjetj )	/* (c) 1998 Seibu Kaihatsu */
	DRIVER( rfjetus )	/* (c) 1998 Seibu Kaihatsu (Fabtek license) */

	/* Tad games (Tad games run on Seibu hardware) */
	DRIVER( cabal )		/* (c) 1988 Tad + Fabtek license */
	DRIVER( cabal2 )	/* (c) 1988 Tad + Fabtek license */
	DRIVER( cabalbl )	/* bootleg */
	DRIVER( toki )		/* (c) 1989 Tad (World) */
	DRIVER( tokia )		/* (c) 1989 Tad (World) */
	DRIVER( tokij )		/* (c) 1989 Tad (Japan) */
	DRIVER( tokiu )		/* (c) 1989 Tad + Fabtek license (US) */
	DRIVER( tokib )		/* bootleg */
	DRIVER( bloodbro )	/* (c) 1990 Tad */
	DRIVER( bloodbra )	/* (c) 1990 Tad */
	DRIVER( weststry )	/* bootleg */
	DRIVER( skysmash )	/* (c) 1990 Nihon System Inc. */
	DRIVER( legionna )	/* (c) 1992 Tad (World) */
	DRIVER( legionnu )	/* (c) 1992 Tad + Fabtek license (US) */
	DRIVER( heatbrl )	/* (c) 1992 Tad (World) */
	DRIVER( heatbrlo )	/* (c) 1992 Tad (World) */
	DRIVER( heatbrlu )	/* (c) 1992 Tad (US) */
	DRIVER( godzilla )	/* (c) 1993 Banpresto */
	DRIVER( denjinmk )	/* (c) 199? Banpresto */
	DRIVER( sdgndmrb )	/* (c) 1993 Banpresto */
	DRIVER( cupsoc )	/* (c) 1992 Seibu */
	DRIVER( cupsoc2 )	/* (c) 1992 Seibu */
	DRIVER( olysoc92 )	/* (c) 1992 Seibu */
	DRIVER( cupsocbl )	/* bootleg */
	DRIVER( goal92 )	/* bootleg */

	/* Jaleco games */
	DRIVER( exerion )	/* (c) 1983 Jaleco */
	DRIVER( exeriont )	/* (c) 1983 Jaleco + Taito America license */
	DRIVER( exerionb )	/* bootleg */
	DRIVER( fcombat )	/* (c) 1985 Jaleco */
	DRIVER( pturn )		/* (c) 1984 Jaleco */
	DRIVER( ddayjlc )	/* (c) 1984 Jaleco */
	DRIVER( ddayjlca )	/* (c) 1984 Jaleco */
	DRIVER( formatz )	/* (c) 1984 Jaleco */
	DRIVER( aeroboto )	/* (c) 1984 Williams */
	DRIVER( citycon )	/* (c) 1985 Jaleco */
	DRIVER( citycona )	/* (c) 1985 Jaleco */
	DRIVER( cruisin )	/* (c) 1985 Jaleco/Kitkorp */
	DRIVER( momoko )	/* (c) 1986 Jaleco */
	DRIVER( argus )		/* (c) 1986 Jaleco */
	DRIVER( valtric )	/* (c) 1986 Jaleco */
	DRIVER( butasan )	/* (c) 1987 Jaleco */
	DRIVER( psychic5 )	/* (c) 1987 Jaleco */
	DRIVER( ginganin )	/* (c) 1987 Jaleco */
	DRIVER( gingania )	/* (c) 1987 Jaleco */
	DRIVER( skyfox )	/* (c) 1987 Jaleco + Nichibutsu USA license */
	DRIVER( exerizrb )	/* bootleg */
	DRIVER( homerun )	/* (c) 1988 Jaleco */
	DRIVER( bigrun )	/* (c) 1989 Jaleco */
	DRIVER( cischeat )	/* (c) 1990 Jaleco */
	DRIVER( f1gpstar )	/* (c) 1991 Jaleco */
	DRIVER( armchmp2 )	/* (c) 1992 Jaleco */
	DRIVER( wildplt )	/* (c) 1992 Jaleco */
	DRIVER( f1gpstr2 )	/* (c) 1993 Jaleco */
	DRIVER( scudhamm )	/* (c) 1994 Jaleco */
	DRIVER( tetrisp2 )	/* (c) 1997 Jaleco */
	DRIVER( teplus2j )	/* (c) 1997 Jaleco */
	DRIVER( stepstag )	/* (c) 1999 Jaleco */
	DRIVER( rockn )		/* (c) 1999 Jaleco */
	DRIVER( rockna )	/* (c) 1999 Jaleco */
	DRIVER( rockn2 )	/* (c) 1999 Jaleco */
	DRIVER( rocknms )	/* (c) 1999 Jaleco */
	DRIVER( rockn3 )	/* (c) 1999 Jaleco */
	DRIVER( rockn4 )	/* (c) 2000 Jaleco */

	/* Jaleco Mega System 1 games */
	DRIVER( lomakai )	/* (c) 1988 (World) */
	DRIVER( makaiden )	/* (c) 1988 (Japan) */
	DRIVER( p47 )		/* (c) 1988 */
	DRIVER( p47j )		/* (c) 1988 (Japan) */
	DRIVER( kickoff )	/* (c) 1988 (Japan) */
	DRIVER( tshingen )	/* (c) 1988 (Japan) */
	DRIVER( tshingna )	/* (c) 1988 (Japan) */
	DRIVER( kazan )		/* (c) 1988 (World) */
	DRIVER( iganinju )	/* (c) 1988 (Japan) */
	DRIVER( astyanax )	/* (c) 1989 */
	DRIVER( lordofk )	/* (c) 1989 (Japan) */
	DRIVER( hachoo )	/* (c) 1989 */
	DRIVER( jitsupro )	/* (c) 1989 (Japan) */
	DRIVER( plusalph )	/* (c) 1989 */
	DRIVER( stdragon )	/* (c) 1989 */
	DRIVER( rodland )	/* (c) 1990 */
	DRIVER( rodlandj )	/* (c) 1990 (Japan) */
	DRIVER( rodlndjb )	/* bootleg */
	DRIVER( avspirit )	/* (c) 1991 */
	DRIVER( phantasm )	/* (c) 1991 (Japan) */
	DRIVER( edf )		/* (c) 1991 */
	DRIVER( edfu )		/* (c) 1991 */
	DRIVER( 64street )	/* (c) 1991 */
	DRIVER( 64streej )	/* (c) 1991 (Japan) */
	DRIVER( soldamj )	/* (c) 1992 (Japan) */
	DRIVER( bigstrik )	/* (c) 1992 */
	DRIVER( bigstrkb )	/* bootleg on different hardware */
	DRIVER( bestleag )	/* bootleg, by Playmark? */
	DRIVER( chimerab )	/* (c) 1993 */
	DRIVER( cybattlr )	/* (c) 1993 */
	DRIVER( peekaboo )	/* (c) 1993 */

	/* Jaleco Mega System 1 games */
	DRIVER( hayaosi1 )	/* (c) 1994 */
	DRIVER( bbbxing )	/* (c) 1994 */
	DRIVER( 47pie2 )	/* (c) 1994 */
	DRIVER( 47pie2o )	/* (c) 1994 */
	DRIVER( desertwr )	/* (c) 1995 */
	DRIVER( gametngk )	/* (c) 1995 */
	DRIVER( tetrisp )	/* (c) 1995 Jaleco / BPS */
	DRIVER( p47aces )	/* (c) 1995 */
	DRIVER( akiss )		/* (c) 1995 */
	DRIVER( gratia )	/* (c) 1996 */
	DRIVER( gratiaa )	/* (c) 1996 */
	DRIVER( kirarast )	/* (c) 1996 */
	DRIVER( tp2m32 )	/* (c) 1997 */
	DRIVER( f1superb )	/* (c) 1994 */

	/* Video System Co. games */
	DRIVER( rabiolep )	/* (c) 1987 V-System Co. (Japan) */
	DRIVER( rpunch )	/* (c) 1987 V-System Co. + Bally/Midway/Sente license (US) */
	DRIVER( svolley )	/* (c) 1989 V-System Co. (Japan) */
	DRIVER( svolleyk )	/* (c) 1989 V-System Co. (Korea) */
	DRIVER( svolleyu )	/* (c) 1989 V-System Co. + Data East license (US) */
	DRIVER( tail2nos )	/* [1989] V-System Co. */
	DRIVER( sformula )	/* [1989] V-System Co. (Japan) */
	DRIVER( ojankoc )	/* [1986] V-System Co. (Japan) */
	DRIVER( ojankoy )	/* [1986] V-System Co. (Japan) */
	DRIVER( ojanko2 )	/* [1987] V-System Co. (Japan) */
	DRIVER( ccasino )	/* [1987] V-System Co. (Japan) */
	DRIVER( ojankohs )	/* [1988] V-System Co. (Japan) */
	DRIVER( nekkyoku )	/* [1988] Video System Co. (Japan) */
	DRIVER( idolmj )	/* [1988] (c) System Service (Japan) */
	DRIVER( mjnatsu )	/* [1989] Video System presents (Japan) */
	DRIVER( natsuiro )	/* [1989] Video System presents (Japan) */
	DRIVER( mfunclub )	/* [1989] V-System (Japan) */
	DRIVER( daiyogen )	/* [1990] Video System Co. (Japan) */
	DRIVER( nmsengen )	/* (c) 1991 Video System (Japan) */
	DRIVER( fromance )	/* (c) 1991 Video System Co. (Japan) */
	DRIVER( pipedrm )	/* (c) 1990 Video System Co. (US) */
	DRIVER( pipedrmj )	/* (c) 1990 Video System Co. (Japan) */
	DRIVER( hatris )	/* (c) 1990 Video System Co. (Japan) */
	DRIVER( spinlbrk )	/* (c) 1990 V-System Co. (World) */
	DRIVER( spinlbru )	/* (c) 1990 V-System Co. (US) */
	DRIVER( spinlbrj )	/* (c) 1990 V-System Co. (Japan) */
	DRIVER( pspikes )	/* (c) 1991 Video System Co. (World) */
	DRIVER( pspikesk )	/* (c) 1991 Video System Co. (Korea) */
	DRIVER( svolly91 )	/* (c) 1991 Video System Co. (Japan) */
	DRIVER( pspikesb )	/* bootleg */
	DRIVER( karatblz )	/* (c) 1991 Video System Co. */
	DRIVER( karatblu )	/* (c) 1991 Video System Co. (US) */
	DRIVER( karatblj )	/* (c) 1991 Video System / Tecmo (Japan) */
	DRIVER( turbofrc )	/* (c) 1991 Video System Co. */
	DRIVER( aerofgt )	/* (c) 1992 Video System Co. */
	DRIVER( aerofgtb )	/* (c) 1992 Video System Co. */
	DRIVER( aerofgtc )	/* (c) 1992 Video System Co. */
	DRIVER( sonicwi )	/* (c) 1992 Video System Co. (Japan) */
	DRIVER( aerfboot )	/* bootleg */
	DRIVER( wbbc97 )	/* (c) 1997 Comad */
	DRIVER( welltris )	/* (c) 1991 Video System Co. (Japan) */
	DRIVER( welltrij )	/* (c) 1991 Video System Co. (Japan) */
	DRIVER( quiz18k )	/* (c) 1992 EIM (Welltris hardware) */
	DRIVER( f1gp )		/* (c) 1991 Video System Co. */
	DRIVER( f1gp2 )		/* (c) 1992 Video System Co. */
	DRIVER( crshrace )	/* (c) 1993 Video System Co. */
	DRIVER( crshrac2 )	/* (c) 1993 Video System Co. */
	DRIVER( taotaido )	/* (c) 1993 Video System Co. */
	DRIVER( taotaida )	/* (c) 1993 Video System Co. */
	DRIVER( gstriker )	/* (c) [1993] Human */
	DRIVER( vgoalsoc )
	DRIVER( vgoalsca )
	DRIVER( twrldc94 )      /* (c) 1994 Tecmo */
	DRIVER( twrdc94a )      /* (c) 1994 Tecmo */
	DRIVER( suprslam )	/* (c) 1995 Banpresto */
	DRIVER( fromanc2 )	/* (c) 1995 Video System Co. (Japan) */
	DRIVER( fromancr )	/* (c) 1995 Video System Co. (Japan) */
	DRIVER( fromanc4 )	/* (c) 1998 Video System Co. (Japan) */
	DRIVER( inufuku )	/* (c) 1998 Video System Co. (Japan) */

	/* Psikyo games */
	DRIVER( samuraia )	/* (c) 1993 (World) */
	DRIVER( sngkace )	/* (c) 1993 (Japan) */
	DRIVER( gunbird )	/* (c) 1994 */
	DRIVER( gunbirdk )	/* (c) 1994 */
	DRIVER( gunbirdj )	/* (c) 1994 */
	DRIVER( btlkroad )	/* (c) 1994 */
	DRIVER( s1945 )		/* (c) 1995 */
	DRIVER( s1945a )	/* (c) 1995 */
	DRIVER( s1945j )	/* (c) 1995 */
	DRIVER( s1945jn )	/* (c) 1995 */
	DRIVER( tengai )	/* (c) 1996 */
	DRIVER( s1945ii )	/* (c) 1997 */
	DRIVER( soldivid )	/* (c) 1997 */
	DRIVER( sbomberb )	/* (c) 1998 */
	DRIVER( daraku )	/* (c) 1998 */
	DRIVER( gunbird2 )	/* (c) 1998 */
	DRIVER( s1945iii )	/* (c) 1999 */
	DRIVER( dragnblz )	/* (c) 2000 */
	/* tgm2  (c) 2000 -- removed at Arika's request */
	/* tgm2p (c) 2000 -- removed at Arika's request */
	DRIVER( gnbarich )	/* (c) 2001 */
	DRIVER( mjgtaste )	/* (c) 2002 */
	DRIVER( hotgmck )	/* (c) 1997 */
	DRIVER( hgkairak )	/* (c) 1998 */
	DRIVER( hotgmck3 )	/* (c) 1999 */
	DRIVER( hotgm4ev )	/* (c) 2000 */
	DRIVER( hotgmcki )	/* (c) 2001 */
	DRIVER( loderndf )	/* (c) 2000 */
	DRIVER( loderdfa )	/* (c) 2000 */
	DRIVER( hotdebut )	/* (c) 2000 */

	/* Orca games */
	DRIVER( marineb )	/* (c) 1982 Orca */
	DRIVER( changes )	/* (c) 1982 Orca */
	DRIVER( changesa )	/* (c) 1982 Orca + Eastern Micro Electronics license */
	DRIVER( looper )	/* (c) 1982 Orca */
	DRIVER( springer )	/* (c) 1982 Orca */
	DRIVER( hoccer )	/* (c) 1983 Eastern Micro Electronics, Inc. */
	DRIVER( hoccer2 )	/* (c) 1983 Eastern Micro Electronics, Inc. */
	DRIVER( bcruzm12 )	/* (c) 1983 Sigma Ent. Inc. */
	DRIVER( hopprobo )	/* (c) 1983 Sega */
	DRIVER( wanted )	/* (c) 1984 Sigma Ent. Inc. */
	DRIVER( funkybee )	/* (c) 1982 Orca */
	DRIVER( skylancr )	/* (c) 1983 Orca */
	DRIVER( skylance )	/* (c) 1983 Orca + Esco Trading Co license */
	DRIVER( zodiack )	/* (c) 1983 Orca + Esco Trading Co license */
	DRIVER( dogfight )	/* (c) 1983 Thunderbolt */
	DRIVER( moguchan )	/* (c) 1982 Orca + Eastern Commerce Inc. license (doesn't appear on screen) */
	DRIVER( percuss )	/* (c) 1981 Orca */
	DRIVER( bounty )	/* (c) 1982 Orca */
	DRIVER( espial )	/* (c) 1983 Thunderbolt, Orca logo is hidden in title screen */
	DRIVER( espialu )	/* (c) 1983 Thunderbolt, Orca logo is hidden in title screen */
	DRIVER( netwars )	/* (c) 1983 Orca + Esco Trading Co license */
	/* Vastar was made by Orca, but when it was finished, Orca had already bankrupted. */
	/* So they sold this game as "Made by Sesame Japan" because they couldn't use */
	/* the name "Orca" */
	DRIVER( vastar )	/* (c) 1983 Sesame Japan */
	DRIVER( vastar2 )	/* (c) 1983 Sesame Japan */
/*
   other Orca games:
   82 Battle Cross                         Kit 2P
   82 River Patrol Empire Mfg/Kerstens Ind Ded 2P        HC Action
   82 Slalom                               Kit 2P        HC Action
   83 Net Wars                                 2P
   83 Super Crush                          Kit 2P           Action
*/

	/* Gaelco 2D games */
	DRIVER( mastboy )	/* (c) 1987 - No Ref on the PCB */
	DRIVER( mastboyi )	/* (c) 1987 - No Ref on the PCB */
	DRIVER( xorworld )	/* (c) 1990 - prototype */
	DRIVER( bigkarnk )	/* (c) 1991 - Ref 901112-1 */
	/* Master Boy 2 */	/* (c) 1991 - Ref ??? */
	DRIVER( splash )	/* (c) 1992 - Ref 922804 */
	DRIVER( splash10 )	/* (c) 1992 - Ref 922804 */
	DRIVER( paintlad )	/* (c) 1992 - ver 1.3 */
	DRIVER( roldfrog )	/* not a gaelco game but based on splash code */
	DRIVER( roldfrga )	/* not a gaelco game but based on splash code */
	DRIVER( rebus )		/* not a gaelco game but on similar hw to splash */
	DRIVER( funystrp )	/* not a gaelco game but on similar hw to splash */
	DRIVER( puckpepl )	/* not a gaelco game but on similar hw to splash */
	DRIVER( thoop )		/* (c) 1992 - Ref 922804/1 */
	DRIVER( squash )	/* (c) 1992 - Ref 922804/2 */
	DRIVER( wrally )	/* (c) 1993 - Ref 930705 */
	DRIVER( wrallya )	/* (c) 1993 - Ref 930705 */
	DRIVER( glass )		/* (c) 1993 - Ref 931021 */
	DRIVER( glass10 )	/* (c) 1993 - Ref 931021 */
	DRIVER( glass10a )	/* (c) 1993 - Ref 931021 */
	DRIVER( targeth )	/* (c) 1994 - Ref 940531 */
	DRIVER( thoop2 )	/* (c) 1994 - Ref ??? */
	DRIVER( aligator )	/* (c) 1994 - Ref 940411 */
	DRIVER( aligatun )	/* (c) 1994 - Ref 940411 (unprotected) */
	DRIVER( biomtoy )	/* (c) 1995 - Ref 922804/2 - (unprotected) */
	DRIVER( touchgo )	/* (c) 1995 - Ref 950906 */
	DRIVER( touchgon )	/* (c) 1995 - Ref 950906 */
	DRIVER( touchgoe )	/* (c) 1995 - Ref 950510-1 */
	DRIVER( wrally2 )	/* (c) 1995 - Ref 950510 */
	DRIVER( maniacsp )	/* (c) 1996 - Ref 922804/2 - (prototype) */
	DRIVER( maniacsq )	/* (c) 1996 - Ref ??? - (unprotected) */
	DRIVER( snowboar )	/* (c) 1996 - Ref 960419/1 */
	DRIVER( snowbalt )	/* (c) 1996 - Ref 960419/1 */
	DRIVER( bang )		/* (c) 1998 - Ref ??? */
	DRIVER( bangj )		/* (c) 1999 - Ref ??? */

	/* Gaelco 3D games */
	DRIVER( speedup )	/* (c) 1996 - Ref ??? */
	DRIVER( surfplnt )	/* (c) 1997 - Ref 971223 */
	DRIVER( radikalb )	/* (c) 1998 - Ref ??? */

	/*
    Remaining Gaelco Games:
    =======================
    1999: Rolling Extreme
    2000: Football Power
    2001: Smashing Drive
    2002: ATV Track
    */

	/* Kaneko games */
	DRIVER( djboy )		/* (c) 1989 Kaneko */
	DRIVER( djboyj )	/* (c) 1989 Kaneko */
	DRIVER( airbustr )	/* (c) 1990 Kaneko + Namco */
	DRIVER( airbustj )	/* (c) 1990 Kaneko + Namco (Japan) */
	DRIVER( galpanic )	/* (c) 1990 Kaneko */
	DRIVER( galpanib )	/* (c) 1990 Kaneko */
	DRIVER( galpania )	/* (c) 1990 Kaneko */
	DRIVER( galpani2 )	/* (c) 1993 Kaneko */
	DRIVER( gpan2qiz )	/* (c) 1993 Kaneko */
	DRIVER( jchan )		/* (c) 1995 Kaneko */
	DRIVER( jchan2 )	/* (c) 1995 Kaneko */
	DRIVER( galpani3 )	/* (c) 1995 Kaneko */

	/* Kaneko "AX System" games */
	DRIVER( berlwall )	/* (c) 1991 Kaneko */
	DRIVER( berlwalt )	/* (c) 1991 Kaneko */
	DRIVER( mgcrystl )	/* (c) 1991 Kaneko (World) */
	DRIVER( mgcrysto )	/* (c) 1991 Kaneko (World) */
	DRIVER( mgcrystj )	/* (c) 1991 Kaneko + distributed by Atlus (Japan) */
	DRIVER( blazeon )	/* (c) 1992 Atlus */
	DRIVER( sandscrp )	/* (c) 1992 Face */
	DRIVER( sandscra )	/* (c) 1992 Face */
	DRIVER( explbrkr )	/* (c) 1992 Kaneko */
	DRIVER( bakubrkr )	/* (c) 1992 Kaneko */
	DRIVER( shogwarr )	/* (c) 1992 Kaneko */
	DRIVER( fjbuster )	/* (c) 1992 Kaneko */
	DRIVER( brapboys )	/* (c) 1992 Kaneko */
	DRIVER( bloodwar )	/* (c) 1994 Kaneko */
	DRIVER( bonkadv )	/* (c) 1994 Kaneko */
	DRIVER( gtmr )		/* (c) 1994 Kaneko */
	DRIVER( gtmra )		/* (c) 1994 Kaneko */
	DRIVER( gtmre )		/* (c) 1994 Kaneko */
	DRIVER( gtmrusa )	/* (c) 1994 Kaneko (US) */
	DRIVER( gtmr2 )		/* (c) 1995 Kaneko */
	DRIVER( gtmr2a )	/* (c) 1995 Kaneko */
	DRIVER( gtmr2u )	/* (c) 1995 Kaneko */

	/* Kaneko "Super Nova System" games */
	DRIVER( skns )
	DRIVER( galpani4 )	/* (c) 1996 Kaneko (Japan) */
	DRIVER( galpanis )	/* (c) 1997 Kaneko (Japan) */
	DRIVER( sengekis )	/* (c) 1997 Kaneko / Warashi (Asia) */
	DRIVER( sengekij )	/* (c) 1997 Kaneko / Warashi (Japan) */
	DRIVER( vblokbrk )	/* (c) 1997 Kaneko / Mediaworks (Asia) */
	DRIVER( sarukani )	/* (c) 1997 Kaneko / Mediaworks (Japan) */
	DRIVER( cyvern )	/* (c) 1998 Kaneko (Japan) */
	DRIVER( galpans2 )	/* (c) 1999 Kaneko (Japan) */
	DRIVER( panicstr )	/* (c) 1999 Kaneko (Japan) */
	DRIVER( senknow )	/* (c) 1999 Kaneko / Kouyousha (Japan) */
	DRIVER( gutsn )		/* (c) 2000 Kaneko / Kouyousha (Japan) */
	DRIVER( puzzloop )	/* (c) 1998 Mitchell (Europe) */
	DRIVER( puzloopj )	/* (c) 1998 Mitchell (Japan) */
	DRIVER( puzloopk )	/* (c) 1998 Mitchell (Korea) */
	DRIVER( puzloopu )	/* (c) 1998 Mitchell (USA) */
	DRIVER( jjparads )	/* (c) 1996 Electro Design Co. (Japan) */
	DRIVER( jjparad2 )	/* (c) 1997 Electro Design Co. (Japan) */
	DRIVER( ryouran )	/* (c) 1998 Electro Design Co. (Japan) */
	DRIVER( teljan )	/* (c) 1999 Electro Design Co. (Japan) */

	/* Seta games */
	DRIVER( hanaawas )	/* (c) SetaKikaku */
	DRIVER( speedatk )	/* CB-0 (c) SetaKikaku */
	DRIVER( srmp2 )		/* UB or UC?? (c) 1987 */
	DRIVER( srmp3 )		/* ZA-0? (c) 1988 */
	DRIVER( mjyuugi )	/* (c) 1990 Visco */
	DRIVER( mjyuugia )	/* (c) 1990 Visco */
	DRIVER( ponchin )	/* (c) 1991 Visco */
	DRIVER( ponchina )	/* (c) 1991 Visco */
	DRIVER( tndrcade )	/* UA-0 (c) 1987 Taito */
	DRIVER( tndrcadj )	/* UA-0 (c) 1987 Taito */
	DRIVER( twineagl )	/* UA-2 (c) 1988 + Taito license */
	DRIVER( downtown )	/* UD-2 (c) 1989 + Romstar or Taito license (DSW) */
	DRIVER( downtow2 )	/* UD-2 (c) 1989 + Romstar or Taito license (DSW) */
	DRIVER( downtowp )	/* UD-2 (c) 1989 + Romstar or Taito license (DSW) */
	DRIVER( usclssic )	/* UE   (c) 1989 + Romstar or Taito license (DSW) */
	DRIVER( calibr50 )	/* UH   (c) 1989 + Romstar or Taito license (DSW) */
	DRIVER( arbalest )	/* UK   (c) 1989 + Jordan, Romstar or Taito license (DSW) */
	DRIVER( metafox )	/* UP   (c) 1989 + Jordan, Romstar or Taito license (DSW) */
	DRIVER( drgnunit )	/* (c) 1989 Athena / Seta + Romstar or Taito license (DSW) */
	DRIVER( wits )		/* (c) 1989 Athena (Visco license) */
	DRIVER( thunderl )	/* (c) 1990 Seta + Romstar or Visco license (DSW) */
	DRIVER( wiggie )	/* (c) 1994 Promat (not genuine seta hardware, but based on thunderl code) */
	DRIVER( rezon )		/* (c) 1991 Allumer */
	DRIVER( rezont )	/* (c) 1991 Allumer / Taito */
	DRIVER( stg )		/* (c) 1991 Athena / Tecmo */
	DRIVER( pairlove )	/* (c) 1991 Athena */
	DRIVER( blandia )	/* (c) 1992 Allumer */
	DRIVER( blandiap )	/* (c) 1992 Allumer */
	DRIVER( blockcar )	/* (c) 1992 Visco */
	DRIVER( qzkklogy )	/* (c) 1992 Tecmo */
	DRIVER( neobattl )	/* (c) 1992 Banpresto / Sotsu Agency. Sunrise */
	DRIVER( umanclub )	/* (c) 1992 Tsuburaya Prod. / Banpresto */
	DRIVER( zingzip )	/* UY   (c) 1992 Allumer + Tecmo */
	DRIVER( atehate )	/* (C) 1993 Athena */
	DRIVER( jjsquawk )	/* (c) 1993 Athena / Able */
	DRIVER( kamenrid )	/* (c) 1993 Toei / Banpresto */
	DRIVER( madshark )	/* (c) 1993 Allumer */
	DRIVER( msgundam )	/* (c) 1993 Banpresto */
	DRIVER( msgunda1 )	/* (c) 1993 Banpresto */
	DRIVER( daioh )		/* (C) 1993 Athena */
	DRIVER( oisipuzl )	/* (c) 1993 SunSoft / Atlus */
	DRIVER( triplfun )	/* bootleg */
	DRIVER( utoukond )	/* (c) 1993 Banpresto + Tsuburaya Prod. */
	DRIVER( qzkklgy2 )	/* (c) 1993 Tecmo */
	DRIVER( wrofaero )	/* (c) 1993 Yang Cheng */
	DRIVER( eightfrc )	/* (c) 1994 Tecmo */
	DRIVER( kiwame )	/* (c) 1994 Athena */
	DRIVER( krzybowl )	/* (c) 1994 American Sammy */
	DRIVER( extdwnhl )	/* (c) 1995 Sammy Japan */
	DRIVER( gundhara )	/* (c) 1995 Banpresto */
	DRIVER( sokonuke )	/* (c) 1995 Sammy Industries */
	DRIVER( zombraid )	/* (c) 1995 American Sammy */
	DRIVER( crazyfgt )	/* (c) 1998 Subsino */

	DRIVER( gundamex )	/* (c) 1994 Banpresto */
	DRIVER( grdians )	/* (c) 1995 Banpresto */
	DRIVER( mj4simai )	/* (c) 1996 Maboroshi Ware */
	DRIVER( myangel )	/* (c) 1996 Namco */
	DRIVER( myangel2 )	/* (c) 1997 Namco */
	DRIVER( pzlbowl )	/* (c) 1999 Nihon System / Moss */
	DRIVER( penbros )	/* (c) 2000 Subsino */

	/* SSV System (Seta, Sammy, Visco) games */
	DRIVER( srmp4 )		/* (c) 1993 Seta */
	DRIVER( srmp4o )	/* (c) 1993 Seta */
	DRIVER( twineag2 )	/* (c) 1994 Seta */
	DRIVER( srmp7 )		/* (c) 1997 Seta */
	DRIVER( dynagear )	/* (c) 1993 Sammy */
	DRIVER( survarts )	/* (c) 1993 Sammy */
	DRIVER( survartu )	/* (c) 1993 Sammy (American) */
	DRIVER( eaglshot )	/* (c) 1994 Sammy */
	DRIVER( hypreact )	/* (c) 1995 Sammy */
	DRIVER( meosism )	/* (c) 1996?Sammy */
	DRIVER( hypreac2 )	/* (c) 1997 Sammy */
	DRIVER( sxyreact )	/* (c) 1998 Sammy */
	DRIVER( sxyreac2 )	/* (c) 1999 Sammy */
	DRIVER( cairblad )	/* (c) 1999 Sammy */
	DRIVER( keithlcy )	/* (c) 1993 Visco */
	DRIVER( drifto94 )	/* (c) 1994 Visco */
	DRIVER( janjans1 )	/* (c) 1996 Visco */
	DRIVER( stmblade )	/* (c) 1996 Visco */
	DRIVER( jsk )		/* (c) 1997 Visco */
	DRIVER( koikois2 ) 	/* (c) 1997 Visco */
	DRIVER( mslider )	/* (c) 1997 Visco / Datt Japan */
	DRIVER( ryorioh )	/* (c) 1998 Visco */
	DRIVER( janjans2 )	/* (c) 2000 Visco */
	DRIVER( vasara )	/* (c) 2000 Visco */
	DRIVER( vasara2 )	/* (c) 2001 Visco */
	DRIVER( vasara2a )	/* (c) 2001 Visco */

	DRIVER( ultrax )	/* (c) 1995 Banpresto */
	DRIVER( gdfs )		/* (c) 1995 Banpresto */

	DRIVER( darkhors )	/* bootleg of Jockey Club II */

	/* ST-0016 based games (ST-0016 is a z80 / gfx / sound all in one) */
	/* plain ST-0016 */
	DRIVER( renju )		/* (c) 1994 Visco */
	DRIVER( nratechu )	/* (c) 1996 Seta */
	/* R3000 + ST-0016 */
	DRIVER( srmp5 )		/* (c) 199? Seta */
	DRIVER( speglsht )	/* (c) 1994 Seta */
	DRIVER( speglsha )	/* (c) 1994 Seta */
	/* V810 + ST-0016 */
	DRIVER( mayjinsn )	/* (c) 1994 Seta */
	DRIVER( mayjisn2 )	/* (c) 1994 Seta */

	/* Seta System S12 */
	DRIVER( srmp6 )		/* (c) 1996 Seta (Japan) */

	/* Seta Aleck64 and similar (N64 based) */
	DRIVER( 11beat )
	DRIVER( mtetrisc )

	/* Atlus games */
	DRIVER( powerins )	/* (c) 1993 Atlus (Japan) */
	DRIVER( powerina )	/* (c) 1993 Atlus (Japan) */
	DRIVER( powerinb )	/* (c) 1993 Atlus (Japan) */
	DRIVER( powernjb )	/* (c) 1993 Atlus (Japan) */
	DRIVER( ohmygod )	/* (c) 1993 Atlus (Japan) */
	DRIVER( naname )	/* (c) 1994 Atlus (Japan) */
	DRIVER( blmbycar )	/* (c) 1994 ABM & Gecas - uses same gfx chip as powerins? */
	DRIVER( blmbycau )	/* (c) 1994 ABM & Gecas - uses same gfx chip as powerins? */
	DRIVER( watrball )	/* (c) 1996 ABM */

	/* Sun Electronics / SunSoft games */
	DRIVER( speakres )	/* [Sun Electronics] */
	DRIVER( stratvox )	/* [1980 Sun Electronics] Taito */
	DRIVER( spacecho )	/* bootleg */
	DRIVER( route16 )	/* (c) 1981 Tehkan/Sun + Centuri license */
	DRIVER( route16a )	/* (c) 1981 Tehkan/Sun + Centuri license */
	DRIVER( route16b )	/* bootleg */
	DRIVER( routex )	/* bootleg */
	DRIVER( ttmahjng )	/* Taito */
	DRIVER( fnkyfish )	/* (c) 1981 Sun Electronics */
	DRIVER( kangaroo )	/* (c) 1982 Sun Electronics */
	DRIVER( kangaroa )	/* 136008           (c) 1982 Atari */
	DRIVER( kangarob )	/* (bootleg) */
	DRIVER( arabian )	/* TVG13 (c) 1983 Sun Electronics */
	DRIVER( arabiana )	/* 136019           (c) 1983 Atari */
	DRIVER( markham )	/* TVG14 (c) 1983 Sun Electronics */
	DRIVER( strnskil )	/* TVG15 (c) 1984 Sun Electronics */
	DRIVER( guiness )	/* TVG15 (c) 1984 Sun Electronics */
	DRIVER( pettanp )	/* TVG16 (c) 1984 Sun Electronics (Japan) */
	DRIVER( ikki )		/* TVG17 (c) 1985 Sun Electronics (Japan) */
	DRIVER( farmer )	/* TVG17 (c) 1985 Sun Electronics */
	DRIVER( shanghai )	/* (c) 1988 Sunsoft (Sun Electronics) */
	DRIVER( shangha2 )	/* (c) 1989 Sunsoft (Sun Electronics) */
	DRIVER( kothello )	/* (c) 1990 Success */
	DRIVER( shangha3 )	/* (c) 1993 Sunsoft */
	DRIVER( heberpop )	/* (c) 1994 Sunsoft / Atlus */
	DRIVER( blocken )	/* (c) 1994 KID / Visco */
/*
Other Sun games
1978 (GT)Block Perfect
1978 (GT)Block Challenger
1979 Galaxy Force
1979 Run Away
1979 Dai San Wakusei (The Third Planet)
1979 Warp 1
1980 Cosmo Police (Cosmopolis?)
1985 Ikki
1993 Saikyou Battler Retsuden
1995 Shanghai Banri no Choujou (ST-V)
1996 Karaoke Quiz Intro DonDon (ST-V)
1998 Astra Super Stars (ST-V)
1998 Shanghai Mateki Buyuu (TPS)
*/

	/* SunA games */
	DRIVER( goindol )	/* (c) 1987 Sun a Electronics */
	DRIVER( goindolu )	/* (c) 1987 Sun a Electronics */
	DRIVER( goindolj )	/* (c) 1987 Sun a Electronics */
	DRIVER( rranger )	/* (c) 1988 SunA + Sharp Image license */
	DRIVER( sranger )	/* (c) 1988 */
	DRIVER( srangerb )	/* bootleg */
	DRIVER( srangerw )	/* (c) 1988 SunA (WDK License) */
	DRIVER( hardhead )	/* (c) 1988 SunA */
	DRIVER( hardhedb )	/* bootleg */
	DRIVER( pop_hh )	/* bootleg */
	DRIVER( sparkman )	/* (c) 1989 SunA */
	DRIVER( starfigh )	/* (c) 1990 SunA */
	DRIVER( hardhea2 )	/* (c) 1991 SunA */
	DRIVER( brickzn )	/* (c) 1992 SunA */
	DRIVER( brickzn3 )	/* (c) 1992 SunA */
	DRIVER( bssoccer )	/* (c) 1996 SunA */
	DRIVER( uballoon )	/* (c) 1996 SunA */

	/* Dooyong games */
	DRIVER( gundealr )	/* (c) 1990 Dooyong */
	DRIVER( gundeala )	/* (c) 1990 Dooyong */
	DRIVER( gundealt )	/* (c) 1990 Tecmo */
	DRIVER( yamyam )	/* (c) 1990 Dooyong */
	DRIVER( wiseguy )	/* (c) 1990 Dooyong */
	DRIVER( lastday )	/* (c) 1990 Dooyong */
	DRIVER( lastdaya )	/* (c) 1990 Dooyong */
	DRIVER( gulfstrm )	/* (c) 1991 Dooyong */
	DRIVER( gulfstr2 )	/* (c) 1991 Dooyong + distributed by Media Shoji */
	DRIVER( pollux )	/* (c) 1991 Dooyong */
	DRIVER( polluxa )	/* (c) 1991 Dooyong */
	DRIVER( polluxa2 )	/* (c) 1991 Dooyong */
	DRIVER( flytiger )	/* (c) 1992 Dooyong */
	DRIVER( bluehawk )	/* (c) 1993 Dooyong */
	DRIVER( bluehawn )	/* (c) 1993 NTC */
	DRIVER( sadari )	/* (c) 1993 NTC */
	DRIVER( gundl94 )	/* (c) 1994 Dooyong */
	DRIVER( primella )	/* (c) 1994 NTC */
	DRIVER( superx )	/* (c) 1994 NTC */
	DRIVER( superxm )	/* (c) 1994 Mitchell */
	DRIVER( rshark )	/* (c) 1995 Dooyong */
	DRIVER( popbingo )	/* (c) 1996 Dooyong */

	/* Tong Electronic games */
	DRIVER( leprechn )	/* (c) 1982 */
	DRIVER( potogold )	/* (c) 1982 */
	DRIVER( leprechp )	/* (c) 1982 */
	DRIVER( piratetr )	/* (c) 1982 */
	DRIVER( beezer )	/* (c) 1982 */
	DRIVER( beezer1 )	/* (c) 1982 */

	/* Comad games */
	DRIVER( pushman )	/* (c) 1990 Comad */
	DRIVER( pushmana )	/* (c) 1990 Comad */
	DRIVER( pushmans )	/* (c) 1990 Comad + American Sammy license */
	DRIVER( bballs )	/* (c) 1991 Comad */
	DRIVER( zerozone )	/* (c) 1993 Comad */
	DRIVER( lvgirl94 )	/* (c) 1994 Comad */
	DRIVER( hotpinbl )	/* (c) 1995 Comad & New Japan System */
	DRIVER( galspnbl )	/* (c) 1996 Comad */
		/* the following ones run on modified Gals Panic hardware */
	DRIVER( fantasia )	/* (c) 1994 Comad & New Japan System */
	DRIVER( supmodel )	/* (c) 1994 Comad & New Japan System */
	DRIVER( newfant )	/* (c) 1995 Comad & New Japan System */
	DRIVER( fantsy95 )	/* (c) 1995 Hi-max Technology Inc. */
	DRIVER( zipzap )	/* (c) 1995 Barko Corp */
	DRIVER( missw96 )	/* (c) 1996 Comad */
	DRIVER( missmw96 )	/* (c) 1996 Comad */
	DRIVER( fantsia2 )	/* (c) 1997 Comad */
	DRIVER( fntsia2a )	/* (c) 1997 Comad */
	DRIVER( galhustl )	/* (c) 1997 ACE International */

	/* Playmark games */
	DRIVER( sslam )		/* (c) 1993 - hack of Namco's Super World Court */
	DRIVER( sslama )	/* (c) 1993 - hack of Namco's Super World Court */
	DRIVER( powerbal )	/* (c) 1994 */
	DRIVER( powerbls )	/* (c) 1994 */
	DRIVER( magicstk )	/* (c) 1995 */
	DRIVER( bigtwin )	/* (c) 1995 */
	DRIVER( wbeachvl )	/* (c) 1995 */
	DRIVER( wbeachv2 )	/* (c) 1995 */
	DRIVER( excelsr )	/* (c) 1995 */
	DRIVER( hotmind )	/* (c) 1995 */
	DRIVER( drtomy )	/* (c) 1993 no copyright message (1993 on pcb), some code taken from 'Big Karnak' */
	DRIVER( sderby )	/* (c) 1996 */
	DRIVER( pmroulet )

	/* Pacific Novelty games */
	DRIVER( sharkatt )	/* (c) 1980 */
	DRIVER( thief )		/* (c) 1981 */
	DRIVER( natodef )	/* (c) 1982 */
	DRIVER( natodefa )	/* (c) 1982 */
	DRIVER( mrflea )	/* (c) 1982 */

	/* Tecfri games */
	DRIVER( holeland )	/* (c) 1984 */
	DRIVER( crzrally )	/* (c) 1985 */
	DRIVER( crzralla )	/* (c) 1985 */
	DRIVER( crzrallg )	/* (c) 1985 */
	DRIVER( speedbal )	/* (c) 1987 */
	DRIVER( sauro )		/* (c) 1987 */
	DRIVER( trckydoc )	/* (c) 1987 */
	DRIVER( trckydca )	/* (c) 1987 */

	/* Metro games */
	DRIVER( karatour )	/* (c) 1992 Mitchell */
	DRIVER( ladykill )	/* Yanyaka + Mitchell license */
	DRIVER( moegonta )	/* Yanyaka (Japan) */
	DRIVER( pangpoms )	/* (c) 1992 */
	DRIVER( pangpomm )	/* (c) 1992 Mitchell / Metro */
	DRIVER( skyalert )	/* (c) 1992 */
	DRIVER( poitto )	/* (c) 1993 Metro / Able Corp. */
	DRIVER( dharma )	/* (c) 1994 */
	DRIVER( lastfort )	/* (c) 1994 */
	DRIVER( lastfero )	/* (c) 1994 */
	DRIVER( lastforg )	/* (c) 1994 */
	DRIVER( toride2g )	/* (c) 1994 */
	DRIVER( torid2gg )	/* (c) 1994 */
	DRIVER( gunmast )	/* (c) 1994 */
	DRIVER( daitorid )	/* (c) 1995 */
	DRIVER( dokyusei )	/* (c) 1995 Make Software / Elf / Media Trading */
	DRIVER( dokyusp )	/* (c) 1995 Make Software / Elf / Media Trading */
	DRIVER( puzzli )	/* (c) 1995 Metro / Banpresto */
	DRIVER( 3kokushi )	/* (c) 1996 Mitchell */
	DRIVER( pururun )	/* (c) 1995 Metro / Banpresto */
	DRIVER( balcube )	/* (c) 1996 */
	DRIVER( mouja )		/* (c) 1996 Etona (Japan) */
	DRIVER( bangball )	/* (c) 1996 Banpresto / Kunihiko Tashiro+Goodhouse */
	DRIVER( batlbubl )	/* (c) 1996 Banpresto / Kunihiko Tashiro+Goodhouse */
	DRIVER( gakusai )	/* (c) 1997 MakeSoft */
	DRIVER( gakusai2 )	/* (c) 1998 MakeSoft */
	DRIVER( blzntrnd )	/* (c) 1994 Human Amusement */
	DRIVER( gstrik2 )	/* (c) 1996 Human Amusement */
	DRIVER( hyprduel )	/* (c) 1993 Technosoft (World) */
	DRIVER( hyprdelj )	/* (c) 1993 Technosoft (Japan) */
	DRIVER( magerror )	/* (c) 199? Technosoft (Japan) */

	/* Venture Line games */
	DRIVER( spcforce )	/* (c) 1980 Venture Line */
	DRIVER( spcforc2 )	/* bootleg */
	DRIVER( meteor )	/* (c) 1981 Venture Line */
	DRIVER( looping )	/* (c) 1982 Venture Line + licensed from Video Games */
	DRIVER( loopinga )	/* (c) 1982 Venture Line + licensed from Video Games */
	DRIVER( skybump )	/* (c) 1982 Venture Line */
	DRIVER( suprridr )	/* (c) 1983 Venture Line + Taito license */

	/* Yun Sung games */
	DRIVER( paradise )	/* (c) >1994 Yun Sung */
	DRIVER( paradlx )	/* (c) >1994 Yun Sung */
	DRIVER( tgtball )	/* (c) 1995 Yun Sung */
	DRIVER( tgtballa )	/* (c) 1995 Yun Sung */
	DRIVER( torus )		/* (c) 1996 Yun Sung */
	DRIVER( madball )	/* (c) 1998 Yun Sung */
	DRIVER( cannball )	/* (c) 1995 Yun Sung / Soft Visio */
	DRIVER( magix )		/* (c) 1995 Yun Sung */
	DRIVER( magicbub )	/* (c) Yun Sung */
	DRIVER( magicbua )	/* (c) Yun Sung */
	DRIVER( shocking )	/* (c) 1997 Yun Sung */
	DRIVER( bombkick )	/* (c) 1998 Yun Sung */
	DRIVER( nmg5 )		/* (c) 1998 Yun Sung */
	DRIVER( searchey )	/* (c) 1999 Yun Sung */
	DRIVER( pclubys )	/* (c) 2000 Yun Sung */
	DRIVER( pclubysa )	/* (c) 2000 Yun Sung */
	DRIVER( garogun )	/* (c) 2000 Yun Sung */
	DRIVER( wondstck )	/* (c) ???? Yun Sung */

	/* Zilec games */
	DRIVER( blueprnt )	/* (c) 1982 Bally Midway (Zilec in ROM 3U, and the programmer names) */
	DRIVER( blueprnj )	/* (c) 1982 Jaleco (Zilec in ROM 3U, and the programmer names) */
	DRIVER( saturn )	/* (c) 1983 Jaleco (Zilec in ROM R6, and the programmer names) */

	/* Fuuki FG-2 games */
	DRIVER( gogomile )	/* (c) 1995 */
	DRIVER( gogomilj )	/* (c) 1995 (Japan) */
	DRIVER( pbancho )	/* (c) 1996 (Japan) */

	/* Fuuki FG-3 games */
	DRIVER( asurabld )	/* (c) 1998 (Japan) */

	/* Unico games */
	DRIVER( drgnmst )	/* (c) 1994 */
	DRIVER( burglarx )	/* (c) 1997 */
	DRIVER( zeropnt )	/* (c) 1998 */
	DRIVER( zeropnta )	/* (c) 1998 */
	DRIVER( zeropnt2 )	/* (c) 1999 */
	DRIVER( silkroad )	/* (c) 1999 */

	/* Afega games */
	DRIVER( twinactn )	/* (c) 1996 - bootleg of mustang in nmk16.c */
	DRIVER( stagger1 )	/* (c) 1998 */
	DRIVER( redhawk )	/* (c) 1997 */
	DRIVER( redhawkb )	/* (c) 1997 */
	DRIVER( grdnstrm )	/* (c) 1998 */
	DRIVER( bubl2000 )	/* (c) 1998 Tuning */
	DRIVER( hotbubl )	/* (c) 1998 Pandora */
	DRIVER( popspops )	/* (c) 1999 */
	DRIVER( firehawk )	/* (c) 2001 ESD */
	DRIVER( mangchi )	/* (c) 2000 */
	DRIVER( spec2k )	/* (c) 2000 Yonatech */

	/* ESD games */
	/* http://www.esdgame.co.kr/english/ */
	DRIVER( multchmp )	/* (c) 1998 (World) */
	DRIVER( multchmk )	/* (c) 1998 (Korea) */
	DRIVER( mchampdx )	/* (c) 1999 ESD */
	DRIVER( mchampda )	/* (c) 1999 ESD */
	DRIVER( hedpanic )	/* (c) 2000 ESD */
	DRIVER( tangtang )	/* (c) 2000 ESD */

	/* Dyna Electronics / Dynax / Nakanihon games */
	DRIVER( royalmah )	/* (c) 1982 Falcon */
	DRIVER( suzume )	/*  ??  (c) 1986 Dyna Electronics */
	DRIVER( hnayayoi )	/* "02" (c) 1987 Dyna Electronics */
	DRIVER( dondenmj )	/* "03" (c) 1986 Dyna Electronics */
	DRIVER( hnfubuki )	/* "06" (c) 1987 Dynax */
	DRIVER( mjdiplob )	/* "07" (c) 1987 Dynax */
	DRIVER( untoucha )	/* "08" (c) 1987 Dynax */
	DRIVER( tontonb )	/* "09" (c) 1987 Dynax */
	DRIVER( hanamai )	/* "16" (c) 1988 Dynax */
	DRIVER( majs101b )	/* "17" (c) [1988] Dynax */
	DRIVER( hnkochou )	/* "20" (c) 1989 Dynax */
	DRIVER( mjderngr )	/* "22" (c) 1989 Dynax */
	DRIVER( hnoridur )	/* "23" (c) 1989 Dynax */
	DRIVER( drgpunch )	/* "24" (c) 1989 Dynax */
	DRIVER( mjfriday )	/* "26" (c) [1989] Dynax */
	DRIVER( jantouki )	/* "27" Jantouki 1989 Dynax */
	DRIVER( mjifb )		/* "29" 1990 Dynax */
	DRIVER( sprtmtch )	/* "31" (c) 1989 Dynax + Fabtek license */
	DRIVER( maya )		/* (c) 1994 Promat */
	DRIVER( inca )		/* (c) 199? ???? */
	DRIVER( mcnpshnt )	/* "33" Mahjong Campus Hunting 1990 Dynax */
	DRIVER( 7jigen )	/* "37" 7jigen no Youseitachi 1990 Dynax */
	DRIVER( neruton )	/* "45" Neruton Haikujiradan 1990 Dynax */
	DRIVER( shpeng )	/* (c) 1995 WSAC Systems */
	DRIVER( mjdialq2 )	/* "52" (c) 1991 Dynax */
	DRIVER( yarunara )	/* "55" Mahjong Yarunara 1991 Dynax */
	DRIVER( mjangels )	/* "61" Mahjong Angels 1991 Dynax */
	DRIVER( quiztvqq )	/* "64" Quiz TV Gassyuukoku Q&Q 1992 Dynax */
	DRIVER( mmpanic )	/* "70" 1992 Nakanihon/Taito */
	DRIVER( quizchq )	/* "73" (c) 1993 Nakanihon */
	DRIVER( quizchql )	/* "73" (c) 1993 Laxan */
	DRIVER( funkyfig )	/* "74" (c) 1993 Nakanihon + East Technology */
	DRIVER( animaljr )	/* "75" 1993 Nakanihon/Taito */
	DRIVER( mjmyster )	/* "?7" 1994 Dynax */
	DRIVER( quiz365 )	/* "78" (c) 1994 Nakanihon */
	DRIVER( quiz365t )	/* "78" (c) 1994 Nakanihon (Taiwan) */
	DRIVER( rongrong )	/* "80" (c) 1994 Nakanihon */
	DRIVER( rongrngg )	/* "80" (c) 1994 Nakanihon */
	DRIVER( mjdchuka )	/*      (c) 1995 Nakanihon */
	DRIVER( nettoqc )	/* "103" (c) 1995 Nakanihon */
	DRIVER( ddenlovr )	/* "113" (c) 1996 Dynax */
	DRIVER( hanakanz )	/* "507" 1996 Dynax */
	DRIVER( hkagerou )	/* "510" Hana Kagerou 1996 Nakanihon */
	DRIVER( realbrk )	/* "600" Billiard Academy Real Break 1998 (Europe)*/
	DRIVER( realbrkk )	/* "600" Billiard Academy Real Break 1998 (Korea)*/
	DRIVER( realbrkj )	/* "523" Billiard Academy Real Break 1998 (Japan)*/
	DRIVER( mjchuuka )	/* "???" (c) 1998 Dynax */
	DRIVER( mjreach1 )	/* "526" Mahjong Reach Ippatsu 1998 Nihon System/Dynax */

	DRIVER( pkgnsh )	/* "505" Pachinko Gindama Shoubu 1998 (Japan) */
	DRIVER( pkgnshdx )	/* "522" Pachinko Gindama Shoubu DX 1998 (Japan) */

	DRIVER( mjelctrn )	/* "???" Mahjong Electron Base parts 2 & 4 */
	DRIVER( mjelct3 )	/* "???" Mahjong Electron Base parts 2 & 3 */
	DRIVER( mjelct3a )	/* "???" Mahjong Electron Base parts 2 & 3 */

	/* Sigma games */
	DRIVER( nyny )		/* (c) 1980 Sigma Ent. Inc. */
	DRIVER( nynyg )		/* (c) 1980 Sigma Ent. Inc. + Gottlieb */
	DRIVER( arcadia )	/* (c) 1982 Sigma Ent. Inc. */
	DRIVER( r2dtank )	/* (c) 1980 Sigma Ent. Inc. */
	DRIVER( spiders )	/* (c) 1981 Sigma Ent. Inc. */
	DRIVER( spiders2 )	/* (c) 1981 Sigma Ent. Inc. */
	DRIVER( spinner )	/* bootleg */

	/* IGS games ( www.igs.com.tw ) */
	DRIVER( goldstar )	/* (c) 198? IGS */
	DRIVER( goldstbl )	/* (c) 198? IGS */
	DRIVER( moonlght )	/* bootleg */
	DRIVER( csk227it )	/* (c) 198? IGS */
	DRIVER( csk234it )	/* (c) 198? IGS */
	DRIVER( iqblock )	/* (c) 1993 */
	DRIVER( grndtour )	/* (c) 1993 */
	DRIVER( cabaret )
	DRIVER( lhb )		/* (c) 1995 */
	DRIVER( chindrag )	/* (c) 1995 */
	DRIVER( chugokur )	/* (c) 1995 */
	DRIVER( chmplst2 )	/* (c) 1996 */
	DRIVER( xymg )		/* (c) 1996 */
	DRIVER( grtwall )	/* (c) 1996 */
	DRIVER( vbowl )		/* (c) 1996 */
	DRIVER( vbowlj )	/* (c) 1996 */
	DRIVER( drgnwrld )	/* (c) 1997 */
	DRIVER( drgwrld3 )	/* (c) 1995 */

	DRIVER( lordgun )	/* (c) 1994 */
	DRIVER( hfh )		/* (c) 199? */
	DRIVER( tarzan )

	/* IGS PGM System Games */
	DRIVER( pgm )
	DRIVER( orlegend )	/* (c) 1997 */
	DRIVER( orlegnde )	/* (c) 1997 */
	DRIVER( orlegndc )	/* (c) 1997 */
	DRIVER( orld111c )	/* (c) 1997 */
	DRIVER( orld105k )	/* (c) 1997 */
	DRIVER( drgw2 )		/* (c) 1997 */
	DRIVER( drgw2c )	/* (c) 1997 */
	DRIVER( drgw2j )	/* (c) 1997 */
	DRIVER( drgw3 )		/* (c) 1998 */
	DRIVER( drgw3k )	/* (c) 1998 */
	DRIVER( kov )		/* (c) 1999 */
	DRIVER( kovplus )	/* (c) 1999 */
	DRIVER( kovplusa )	/* (c) 1999 */
	DRIVER( kov115 )	/* (c) 1999 */
	DRIVER( kovj )		/* (c) 1999 */
	DRIVER( kovsh )		/* (c) 1999 */
	DRIVER( photoy2k )	/* (c) 1999 */
	DRIVER( raf102j )	/* (c) 1999 */
	DRIVER( killbld )	/* (c) 1998 */
	DRIVER( killbldt )	/* (c) 1998 */
	DRIVER( puzlstar )	/* (c) 1999 */
	DRIVER( olds )		/* (c) 1999 */
	DRIVER( olds100 )	/* (c) 1999 */
	DRIVER( olds100a )	/* (c) 1999 */
  	DRIVER( kov2 )		/* (c) 2000 */
 	DRIVER( kov2106 )	/* (c) 2000 */
  	DRIVER( kov2p )		/* (c) 2000 */
	DRIVER( ddp2 )		/* (c) 2001 */
	DRIVER( puzzli2 )	/* (c) 2001 */
	DRIVER( martmast )	/* (c) 2001 */
	DRIVER( theglad )	/* (c) 2001 */
	DRIVER( dmnfrnt )	/* (c) 2002 */
	DRIVER( dmnfrnta )	/* (c) 2002 */

	/* AdvanceMAME new games from ShmupMAME v3.0b */
	/* Cave PGM System Games */
	DRIVER( ket )
	DRIVER( keta )
	DRIVER( ketb )
	DRIVER( ddp3 )
	DRIVER( ddp3a )
	DRIVER( ddp3b )
	DRIVER( ddp3blk )
	DRIVER( espgal )

	/* RamTek games */
	DRIVER( hitme )		/* (c) 1976 Ramtek */
	DRIVER( mblkjack )	/* (c) 197? Mirco */
	DRIVER( barricad )	/* (c) 1976 Ramtek */
	DRIVER( brickyrd )	/* (c) 1976 Ramtek */
	DRIVER( starcrus )	/* (c) 1977 Ramtek */

	/* Omori games */
	DRIVER( shuttlei )	/* (c) 197? Omori */
	DRIVER( battlex )	/* (c) 1982 Omori E. Co., Ltd. */
	DRIVER( carjmbre )	/* (c) 1983 Omori Electric Co., Ltd. */
	DRIVER( popper )	/* (c) 1983 Omori Electric Co., Ltd. */

	/* TCH games */
	DRIVER( speedspn )	/* (c) 1994 */
	DRIVER( mstworld )	/* (c) 1994 */

	DRIVER( kickgoal )	/* (c) 1995 */
	DRIVER( actionhw )	/* (c) 1995 */

	/* U.S. Games games */
	DRIVER( usg32 )		/* (c) 1987 */
	DRIVER( usg82 )		/* (c) 1988 */
	DRIVER( usg83 )		/* (c) 1988 */
	DRIVER( usg83x )	/* (c) 1988 */
	DRIVER( usg182 )	/* (c) 1989 */
	DRIVER( usg185 )	/* (c) 1991 */
	DRIVER( usg252 )	/* (c) 1992 */

	/* Sanritsu games */
	DRIVER( mermaid )	/* (c) 1982 Rock-ola,  made by Sanritsu */
	DRIVER( yachtmn )	/* (c) 1982 [Sanritsu] Esco */
	DRIVER( rougien )	/* (c) 1982 Sanritsu */
	DRIVER( drmicro )	/* (c) 1983 Sanritsu */
	DRIVER( appoooh )	/* (c) 1984 Sega,  made by Sanritsu */
	DRIVER( robowres )	/* (c) 1986 Sega */
	DRIVER( robowrb )	/* bootleg */
	DRIVER( bankp )		/* (c) 1984 Sega,  made by Sanritsu */
	DRIVER( mjkjidai )	/* (c) 1986 Sanritsu */
	DRIVER( mayumi )	/* (c) 1988 Victory L.L.C.,  made by Sanritsu */

	/* Rare games */
	DRIVER( xtheball )	/* (c) 1991 Rare */
	DRIVER( btoads )	/* (c) 1994 Rare */
	DRIVER( kinst )		/* (c) 1994 Rare */
	DRIVER( kinst14 )	/* (c) 1994 Rare */
	DRIVER( kinst13 )	/* (c) 1994 Rare */
	DRIVER( kinstp )	/* (c) 1994 Rare */
	DRIVER( kinst2 )	/* (c) 1995 Rare */
	DRIVER( kinst2k )	/* (c) 1995 Rare */
	DRIVER( kinst213 )	/* (c) 1995 Rare */
	DRIVER( kinst211 )	/* (c) 1995 Rare */
	DRIVER( kinst210 )	/* (c) 1995 Rare */

	/* Nihon System games */
	DRIVER( gigasb )	/* (c) 1986 bootleg */
	DRIVER( gigasm2b )	/* (c) 1986 bootleg */
	DRIVER( oigas )		/* (c) 1986 bootleg */
	DRIVER( pbillrd )	/* (c) 1987 Nihon System */
	DRIVER( pbillrds )	/* (c) 1988 Nihon System (+ optional Sega) */
	DRIVER( freekick )	/* (c) 1988 Nihon System (+ optional Sega) */
	DRIVER( freekckb )	/* (c) 1987 bootleg */
	DRIVER( countrun )	/* (c) 1988 Nihon System (+ optional Sega) */
	DRIVER( countrnb )	/* (c) 1988 bootleg */
	DRIVER( countrb2 )	/* (c) 1988 bootleg */

	/* Alba games */
	DRIVER( rmhaihai )	/* (c) 1985 Alba */
	DRIVER( rmhaihib )	/* (c) 1985 Alba */
	DRIVER( rmhaijin )	/* (c) 1986 Alba */
	DRIVER( rmhaisei )	/* (c) 1986 Visco */
	DRIVER( themj )		/* (c) 1987 Visco */
	DRIVER( hanaroku )	/* (c) 1988 Alba */
	DRIVER( yumefuda )	/* (c) 198? Alba */

	/* Home Data games */
	DRIVER( hourouki ) 	/* (c) 1987 Home Data */
	DRIVER( mhgaiden ) 	/* (c) 1987 Home Data */
	DRIVER( mjhokite ) 	/* (c) 1988 Home Data */
	DRIVER( mjclinic ) 	/* (c) 1988 Home Data */
	DRIVER( mrokumei ) 	/* (c) 1988 Home Data */
	DRIVER( reikaids ) 	/* (c) 1988 Home Data */
	DRIVER( mjkojink ) 	/* (c) 1989 Home Data */
	DRIVER( vitaminc ) 	/* (c) 1989 Home Data */
	DRIVER( mjyougo ) 	/* (c) 1989 Home Data */
	DRIVER( lemnangl ) 	/* (c) 1990 Home Data */
	DRIVER( mjkinjas ) 	/* (c) 1991 Home Data */
	DRIVER( battlcry ) 	/* (c) 1991 Home Data */
	DRIVER( jogakuen )	/* Windom corporation */
	DRIVER( mjikaga )	/* Mitchell */

	/* Art & Magic games */
	DRIVER( ultennis )	/* (c) 1993 */
	DRIVER( cheesech )	/* (c) 1994 */
	DRIVER( stonebal )	/* (c) 1994 */
	DRIVER( stoneba2 )	/* (c) 1994 */

	/* Taiyo games */
	DRIVER( dynamski )	/* (c) 1984 Taiyo */
	DRIVER( chinhero )	/* (c) 1984 Taiyo */
	DRIVER( chinher2 )	/* (c) 1984 Taiyo */
	DRIVER( shangkid )	/* (c) 1985 Taiyo + Data East license */
	DRIVER( hiryuken )	/* (c) 1985 Taito */

	/* Tatsumi Games */
	DRIVER( lockon )	/* (c) 1986 */
	DRIVER( apache3 )	/* (c) 1988 */
	DRIVER( roundup5 )	/* (c) 1989 */
	DRIVER( cyclwarr )	/* (c) 1991 */

	/* Electronic Devices */
	DRIVER( mugsmash )	/* (c) Electronic Devices (Italy) / 3D Games (England) */
	DRIVER( stlforce )	/* (c) 1994 Electronic Devices (Italy) / Ecogames S.L. (Spain) */
	DRIVER( fantland )	/* (c) 198? Electronic Devices Italy */
	DRIVER( fantlnda )	/* (c) 198? Electronic Devices Italy */
	DRIVER( galaxygn )	/* (c) 1989 Electronic Devices Italy */
	DRIVER( borntofi )	/* (c) International Games? */
	DRIVER( diverboy )	/* (c) 1992 Electronic Devices */
	DRIVER( twinbrat )	/* (c) 1995 */
	DRIVER( ppmast93 )	/* (c) 1993 Electronic Devices S.R.L. */
	DRIVER( twins )		/* (c) 1994 */
	DRIVER( mwarr )

	/* Arcadia System */
	DRIVER( ar_bios )
	DRIVER( ar_airh )	/* (c) 1988 */
	DRIVER( ar_bowl )	/* (c) 1988 */
	DRIVER( ar_dart )	/* (c) 1987 */
	DRIVER( ar_fast )	/* (c) 1988 */
	DRIVER( ar_ldrb )	/* (c) 1988 */
	DRIVER( ar_ldrba )	/* (c) 1988 */
	DRIVER( ar_ninj )	/* (c) 1987 */
	DRIVER( ar_rdwr )	/* (c) 1988 */
	DRIVER( ar_sdwr )	/* (c) 1988 */
	DRIVER( ar_socc )	/* (c) 1989 */
	DRIVER( ar_spot )	/* (c) 1990 */
	DRIVER( ar_sprg )	/* (c) 1987 */
	DRIVER( ar_xeon )	/* (c) 1988 */

	/* Sente Super System */
	DRIVER( mquake )

	/* Other Amiga-based games */
	DRIVER( upscope )

	/* Crystal System */
	DRIVER( crysbios )
	DRIVER( crysking )	/* 2001 Brezzasoft. Crystal of the kings */
	DRIVER( evosocc )	/* 2001 Evoga. Evolution Soccer */

	/* Scorpion 2+Video Board Games */
	DRIVER( qntoondo )	/* 1993 */
	DRIVER( quintoon )	/* 1993 */
	DRIVER( qntoond )	/* 1993 */
	DRIVER( pokio )		/* 1994 */
	DRIVER( slotsnl )	/* 1995 */
	DRIVER( paradice )	/* 1995 */
	DRIVER( pyramid )	/* 1996 */
	DRIVER( sltblgtk )	/* 1996 */
	DRIVER( sltblgpo )	/* 1996 */
	DRIVER( sltblgp1 )	/* 1996 */
	DRIVER( gldncrwn )	/* 1997 */

	/* MPU4+Video Board Games */
	DRIVER( bctvidbs )
	DRIVER( crmaze )	/* 1994 */
	DRIVER( crmazea )	/* 1994 */
	DRIVER( crmazeb )	/* 1994 */
	DRIVER( turnover )	/* 199? */
	DRIVER( skiltrek )	/* 1992 */
	DRIVER( mating )	/* 199? */
	DRIVER( matinga )	/* 199? */

	/* Other Games */
	DRIVER( astinvad )	/* (c) 1980 Stern */
	DRIVER( kosmokil )	/* bootleg */
	DRIVER( kamikaze )	/* Leijac Corporation */
	DRIVER( spcking2 )
	DRIVER( spaceint )	/* [1980] Shoei */
	DRIVER( spaceinj )	/* [1980] Shoei */
	DRIVER( spacefb )	/* (c) [1980?] Nintendo */
	DRIVER( spacefbg )	/* 834-0031 (c) 1980 Gremlin */
	DRIVER( spacefbb )	/* bootleg */
	DRIVER( spacebrd )	/* bootleg */
	DRIVER( spacedem )	/* (c) 1980 Fortrek + made by Nintendo */
	DRIVER( omegrace )	/* (c) 1981 Midway */
	DRIVER( deltrace )	/* bootleg */
	DRIVER( dday )		/* (c) 1982 Olympia */
	DRIVER( ddayc )		/* (c) 1982 Olympia + Centuri license */
	DRIVER( hexa )		/* D. R. Korea */
	DRIVER( stactics )	/* [1981 Sega] */
	DRIVER( exterm )	/* (c) 1989 Premier Technology - a Gottlieb game */
	DRIVER( kingofb )	/* (c) 1985 Woodplace Inc. */
	DRIVER( ringking )	/* (c) 1985 Data East USA */
	DRIVER( ringkin2 )	/* (c) 1985 Data East USA */
	DRIVER( ringkin3 )	/* (c) 1985 Data East USA */
	DRIVER( ringkinw )	/* (c) 1985 Woodplace Inc. */
	DRIVER( ambush )	/* (c) 1983 Nippon Amuse Co-Ltd */
	DRIVER( ambusht )	/* (c) 1983 Tecfri */
	DRIVER( ambushv )	/* (c) 1983 Volt Elec co-ltd */
	DRIVER( homo )		/* bootleg */
	DRIVER( dlair )
	DRIVER( aztarac )	/* (c) 1983 Centuri (vector game) */
	DRIVER( mole )		/* (c) 1982 Yachiyo Electronics, Ltd. */
	DRIVER( thehand )	/* (c) 1981 T.I.C. */
	DRIVER( gotya )		/* (c) 1981 Game-A-Tron */
	DRIVER( mrjong )	/* (c) 1983 Kiwako */
	DRIVER( crazyblk )	/* (c) 1983 Kiwako + ECI license */
	DRIVER( blkbustr )	/* (c) 1983 Kiwako + ECI license */
	DRIVER( polyplay )
	DRIVER( amspdwy )	/* no copyright notice, but (c) 1987 Enerdyne Technologies, Inc. */
	DRIVER( amspdwya )	/* no copyright notice, but (c) 1987 Enerdyne Technologies, Inc. */
	DRIVER( othldrby )	/* (c) 1995 Sunwise */
	DRIVER( mosaic )	/* (c) 1990 Space */
	DRIVER( mosaica )	/* (c) 1990 Space + Fuuki license */
	DRIVER( gfire2 )	/* (c) 1992 Topis Corp */
	DRIVER( tx1 )		/* (c) 1983 Tatsumi */
	DRIVER( tx1a )		/* (c) 1983 Tatsumi */
	DRIVER( buggyboy )	/* (c) 1985 Tatsumi */
	DRIVER( buggyb1 )	/* (c) 1985 Tatsumi */
	DRIVER( sprcros2 )	/* (c) 1986 GM Shoji */
	DRIVER( sprcrs2a )	/* (c) 1986 GM Shoji */
	DRIVER( gcpinbal )	/* (c) 1994 Excellent System */
	DRIVER( vmetal )	/* (c) 1995 Excellent System? */
	DRIVER( vmetaln )	/* (c) 1995 Excellent System? */
	DRIVER( aquarium )	/* (c) 1996 Excellent System */
	DRIVER( policetr )	/* (c) 1996 P&P Marketing */
	DRIVER( polict11 )	/* (c) 1996 P&P Marketing */
	DRIVER( polict10 )	/* (c) 1996 P&P Marketing */
	DRIVER( plctr13b )	/* (c) 1996 P&P Marketing */
	DRIVER( sshooter )	/* (c) 1998 P&P Marketing */
	DRIVER( sshoot12 )	/* (c) 1998 P&P Marketing */
	DRIVER( sshoot11 )	/* (c) 1998 P&P Marketing */
	DRIVER( pass )		/* (c) 1992 Oksan */
	DRIVER( news )		/* "Virus"??? ((c) 1993 Poby in ROM VIRUS.4) */
	DRIVER( newsa )		/* "Virus"??? ((c) 1993 Poby in ROM VIRUS.4) */
	DRIVER( taxidrvr )	/* [1984 Graphic Techno] */
	DRIVER( xyonix )	/* [1989 Philko] */
	DRIVER( gt507uk )	/* (c) 1986 Grayhound Electronics */
	DRIVER( gt103 )		/* (c) 1986 Grayhound Electronics */
	DRIVER( gt5 )		/* (c) 1984 Grayhound Electronics */
	DRIVER( gt103a )	/* (c) 1984 Greyhound Electronics */
	DRIVER( gt103a1 )	/* (c) 1984 Greyhound Electronics */
	DRIVER( gt103a2 )	/* (c) 1984 Greyhound Electronics */
	DRIVER( gt103a3 )	/* (c) 1984 Greyhound Electronics */
	DRIVER( gt103asx )	/* (c) 1984 Greyhound Electronics */
	DRIVER( gt103aa )	/* (c) 1984 Greyhound Electronics */
	DRIVER( gt103ab )	/* (c) 1984 Greyhound Electronics */
	DRIVER( findout )	/* (c) 1987 [Elettronolo] */
	DRIVER( quiz )		/* (c) 1986 Italian bootleg */
	DRIVER( quiz211 )	/* (c) 1986 Italian bootleg */
	DRIVER( gs4002 )	/* (c) 1982 G.E.I. */
	DRIVER( gs4002a )	/* (c) 1982 G.E.I. */
	DRIVER( gepoker )	/* (c) 1984 Greyhound Electronics */
	DRIVER( gepoker1 )	/* (c) 1984 Greyhound Electronics */
	DRIVER( gepoker2 )	/* (c) 1984 Greyhound Electronics */
	DRIVER( gepoker3 )	/* (c) 1984 Greyhound Electronics */
	DRIVER( gt102c )	/* (c) 1984 Greyhound Electronics */
	DRIVER( gt102b )	/* (c) 1984 Greyhound Electronics */
	DRIVER( gt102c1 )	/* (c) 1985 Greyhound Electronics */
	DRIVER( gt102c2 )	/* (c) 1985 Greyhound Electronics */
	DRIVER( gt102c3 )	/* (c) 1985 Greyhound Electronics */
	DRIVER( sextriv1 )	/* (c) 1985 Kinky Kit and Game Co. */
	DRIVER( sextriv2 )	/* (c) 1985 Kinky Kit and Game Co. */
	DRIVER( dribling )	/* (c) 1983 Model Racing */
	DRIVER( driblino )	/* (c) 1983 Olympia */
	DRIVER( ace )		/* [1976 Allied Leisure] */
	DRIVER( clayshoo )	/* [1979 Allied Leisure] */
	DRIVER( pirates )	/* (c) 1994 NIX */
	DRIVER( genix )		/* (c) 199? NIX */
	DRIVER( fitfight )	/* bootleg of Art of Fighting */
	DRIVER( histryma )	/* bootleg of Fighter's History */
	DRIVER( bbprot )
	DRIVER( flower )	/* (c) 1986 Komax */
	DRIVER( flowerbl )	/* bootleg but (c) 1986 Sega/Alpha */
	DRIVER( beaminv )	/* Tekunon Kougyou */
	DRIVER( mcatadv )	/* (c) 1993 Wintechno */
	DRIVER( mcatadvj )	/* (c) 1993 Wintechno */
	DRIVER( catt )		/* (c) 1993 Wintechno */
	DRIVER( nost )		/* (c) 1993 Face */
	DRIVER( nostj )		/* (c) 1993 Face */
	DRIVER( nostk )		/* (c) 1993 Face */
	DRIVER( 4enraya )	/* (c) 1990 IDSA */
	DRIVER( oneshot )	/* no copyright notice */
	DRIVER( maddonna )	/* (c) 1995 Tuning */
	DRIVER( maddonnb )	/* (c) 1995 Tuning */
	DRIVER( tugboat )	/* (c) 1982 ETM */
	DRIVER( noahsark )	/* (c) 1983 Enter-Tech */
	DRIVER( gotcha )	/* (c) 1997 Dongsung + "presented by Para" */
	DRIVER( ppchamp )	/* (c) 1997 Dongsung + "presented by Para" */
	DRIVER( amerdart )	/* (c) 1989 Ameri Corporation */
	DRIVER( amerdar2 )	/* (c) 1989 Ameri Corporation */
	DRIVER( coolpool )	/* (c) 1992 Catalina Games */
	DRIVER( 9ballsht )	/* (c) 1993 E-Scape EnterMedia + "marketed by Bundra Games" */
	DRIVER( 9ballsh2 )	/* (c) 1993 E-Scape EnterMedia + "marketed by Bundra Games" */
	DRIVER( 9ballsh3 )	/* (c) 1993 E-Scape EnterMedia + "marketed by Bundra Games" */
	DRIVER( gumbo )		/* (c) 1994 Min Corp. */
	DRIVER( mspuzzle )	/* (c) 1994 Min Corp. */
	DRIVER( mspuzzln )	/* (c) 1994 Min Corp. */
	DRIVER( msbingo )	/* (C) 1994 Min Corp. */
	DRIVER( dblpoint )	/* (c) 1995 Dong Bang Electron */
	DRIVER( hangman )	/* (c) 1984 Status Games */
	DRIVER( trivquiz )	/* (c) 1984 Status Games */
	DRIVER( statriv2 )	/* (c) 1984 Status Games */
	DRIVER( statriv4 )	/* (c) 1985 Status Games */
	DRIVER( sextriv )	/* (c) 1985 Status Games */
	DRIVER( quaquiz2 )	/* (c) 1985 Status Games */
	DRIVER( supertr2 )	/* (c) 1986 Status Games */
	DRIVER( supertr3 )	/* (c) 1986 Status Games */
	DRIVER( tickee )	/* (c) 1994 Raster Elite */
	DRIVER( ghoshunt )	/* (c) 1995? Hanaho Games */
	DRIVER( tutstomb )	/* (c) 199?? Island Design */
	DRIVER( crgolf )	/* (c) 1984 Nasco Japan */
	DRIVER( crgolfa )	/* (c) 1984 Nasco Japan */
	DRIVER( crgolfb )	/* (c) 1984 Nasco Japan */
	DRIVER( crgolfc )	/* (c) 1984 Nasco Japan */
	DRIVER( crgolfhi )	/* (c) 1984 Nasco Japan */
	DRIVER( truco )		/* (c) 198? Playtronic SRL */
	DRIVER( thedeep )	/* (c) 1987 Woodplace */
	DRIVER( rundeep )	/* (c) 1988 Cream (bootleg?) */
	DRIVER( wallc )		/* (c) 1984 Midcoin */
	DRIVER( wallca )	/* (c) 1984 Midcoin */
	DRIVER( wink )		/* (c) 1985 Midcoin */
	DRIVER( winka )		/* (c) 1985 Midcoin */
	DRIVER( skyarmy )	/* (c) 1982 Shoei */
	DRIVER( lethalj )	/* (c) 1996 The Game Room */
	DRIVER( eggventr )	/* (c) 1997 The Game Room */
	DRIVER( eggvent7 )	/* (c) 1997 The Game Room */
	DRIVER( eggvntdx )	/* (c) 1997 The Game Room */
	DRIVER( rotaryf )
	DRIVER( sbugger )	/* (c) 1981 Game-A-Tron */
	DRIVER( portrait )	/* (c) 1983 Olympia */
	DRIVER( portrata )	/* (c) 1983 Olympia */
	DRIVER( enigma2 )	/* (c) 1981 Game Plan (Zilec Electronics license) */
	DRIVER( enigma2a )	/* (c) 1984 Zilec Electronics (bootleg?) */
	DRIVER( ltcasino )	/* (c) 1982 Digital Controls Inc */
	DRIVER( mv4in1 )	/* (c) 1983 Entertainment Enterprises */
	DRIVER( ltcasin2 )	/* (c) 1984 Digital Controls Inc */
	DRIVER( coolmini )	/* (c) 1999 Semicom */
	DRIVER( luplup )	/* (c) 1999 Omega System */
	DRIVER( luplup29 )	/* (c) 1999 Omega System */
	DRIVER( puzlbang )	/* (c) 1999 Omega System */
	DRIVER( suplup )	/* (c) 1999 Omega System */
	DRIVER( vamphalf )	/* (c) 1999 DanBi & F2 System */
	DRIVER( misncrft )	/* (c) 2000 Sun */
	DRIVER( hidnctch )	/* (c) 1998 Eolith */
	DRIVER( puzzlekg )	/* (c) 1998 Eolith */
	DRIVER( hidctch2 )	/* (c) 1999 Eolith */
	DRIVER( nhidctch )	/* (c) 1999 Eolith */
	DRIVER( landbrk )	/* (c) 1999 Eolith */
	DRIVER( landbrka )	/* (c) 1999 Eolith */
	DRIVER( raccoon )	/* (c) 1999 Eolith */
	DRIVER( fort2b )	/* (c) 2001 Eolith */
	DRIVER( fort2ba )	/* (c) 2001 Eolith */
	DRIVER( klondkp )	/* (c) 1999 Eolith */
	DRIVER( xfiles )	/* (c) 1999 dgPIX Entertainment Inc */
	DRIVER( kdynastg )	/* (c) 1999 EZ Graphics */
	DRIVER( fmaniac3 )	/* (c) 2002 Saero Entertainment */
	DRIVER( promutrv )	/* (c) 1985 Enerdyne Technologies Inc */
	DRIVER( promutra )	/* (c) 1985 Enerdyne Technologies Inc */
	DRIVER( promutrb )	/* (c) 1985 Enerdyne Technologies Inc */
	DRIVER( strvmstr )	/* (c) 1986 Enerdyne Technologies Inc */
	DRIVER( dorachan )	/* (c) 1980 Craul Denshi */
	DRIVER( ladyfrog )	/* (c) 1990 Mondial Games */
	DRIVER( rabbit )	/* (c) 1997 Electronic Arts */
	DRIVER( tmmjprd )	/* (c) 1997 Media / Sonnet */
	DRIVER( malzak )	/* (c) 1980? Kitronix */
	DRIVER( malzak2 )	/* (c) 1980? Kitronix */
	DRIVER( supertnk )	/* (c) 1981 VIDEO GAMES GmbH, W.-GERMANY */
	DRIVER( crospang )	/* (c) 1998 F2 System */
	DRIVER( funybubl )	/* (c) 1999 Comad Industries */
	DRIVER( dcheese )	/* (c) 1993 HAR */
	DRIVER( lottof2 )	/* (c) 1993 HAR */
	DRIVER( fredmem )	/* (c) 1993 HAR */
	DRIVER( f15se )		/* (c) 1991 Microprose */
	DRIVER( botss )		/* (c) 1992 Microprose */
	DRIVER( stankatk )	/* (c) 1992 Microprose */
	DRIVER( dynadice )	/* ? */
	DRIVER( ssingles )	/* Yachiyo? */
	DRIVER( tcl )		/* (c) 1995 Uniwang */

	/* Multi Amenity Cassette System */
	DRIVER( macsbios )
	DRIVER( kisekaem )	/* (c) 1995 I'Max */
	DRIVER( kisekaeh )	/* (c) 1995 I'Max */
	DRIVER( cultname )	/* (c) 1995 I'Max */
	DRIVER( yuka )		/* (c) 1999 Yubis */
	DRIVER( yujan )		/* (C) 1999 Yubis */

	DRIVER( onetwo )	/* (c) 1997 Barko */
	DRIVER( 1945kiii )	/* (c) 2000 Oriental */
	DRIVER( witch )		/* (c) 1992 Sega / Vic Tokai / Excellent Systems */
	DRIVER( pbchmp95 )	/* (c) 1995 Veltmeijer Automaten */
	DRIVER( cardline )	/* (c) 199? Veltmeijer */
	DRIVER( bmcbowl )	/* (c) 1994 BMC */
	DRIVER( pitboss )	/* (c) 1983 Merit */
	DRIVER( trvwhzho )	/* (c) 1985 Merit */
	DRIVER( trvwhzha )	/* (c) 1985 Merit */
	DRIVER( trvwhzve )	/* (c) 1985 Merit */
	DRIVER( trvwhzva )	/* (c) 1985 Merit */
	DRIVER( trvwhzii )	/* (c) 1985 Merit */
	DRIVER( trvwhziv )	/* (c) 1985 Merit */
	DRIVER( tictac )	/* (c) 1985 Merit */
	DRIVER( phrcraze )	/* (c) 1986 Merit */
	DRIVER( phrcrazs )	/* (c) 1986 Merit */
	DRIVER( couple )	/* (c) 1988 Merit */
	DRIVER( couplep )	/* (c) 1988 Merit */
	DRIVER( couplei )	/* (c) 1988 Merit */
	DRIVER( suprgolf )	/* (c) 19?? Nasco */
	DRIVER( hotblock )	/* (c) 1993 Nics? / Nix? */
	DRIVER( pntnpuzl )	/* Century? */
	DRIVER( trucocl )	/* (c) 1991 Miky SRL */
	DRIVER( ertictac )	/* (c) 1992 Sisteme */
	DRIVER( poizone )   /* (c) 1990 Sisteme */
	DRIVER( ssfindo )	/* (c) 1999 Icarus */
	DRIVER( ppcar )		/* (c) 1999 Icarus */
	DRIVER( monzagp )	/* (c) 1981 Leante Games */
	DRIVER( dambust )	/* (c) 19?? GAT */
	DRIVER( dambustr )	/* (c) 19?? GAT */
	DRIVER( gunpey )	/* (c) 2000 Banpresto */
	DRIVER( spaceg )	/* (c) 19?? */
	DRIVER( wldarrow )	/* (c) 19?? */
	DRIVER( unkmeyco )	/* (c) 19?? */
	DRIVER( laserbas )	/* (c) 1981 Amstar/HOEI */
	DRIVER( laserbsa )	/* (c) 1981 Amstar/HOEI */
	DRIVER( futflash )	/* (c) 19?? HOEI */
	DRIVER( turbosub )	/* (c) 1986 Entertainment Sciences */
	DRIVER( pipeline )	/* (c) 1990 Daehyun Electronics */
	DRIVER( ttchamp )
	DRIVER( ttchampa )
	DRIVER( dreamwld )	/* (c) 2000 SemiCom */
	DRIVER( cybertnk )	/* (c) 1990 Coreland */
	DRIVER( pkscram )	/* (c) 1993 Cosmo Electronics Corporation */
	DRIVER( jollycrd )	/* (c) 1985 TAB-Austria */
	DRIVER( jolycdcr )	/* (c) 1993 Soft Design */
	DRIVER( jolycdit )	/* 199? bootleg? */
	DRIVER( jolycdat )	/* (c) 1986 Fun World */
	DRIVER( jolycdab )	/* (c) 1990 Inter Games */
	DRIVER( bigdeal )	/* (c) 1986 Fun World */
	DRIVER( bigdealb )	/* (c) 1986 Fun World */
	DRIVER( cuoreuno )	/* (c) 1997 bootleg? */
	DRIVER( elephfam )	/* (c) 1997 bootleg? */
	DRIVER( royalcrd )	/* (c) 1991 TAB-Austria */
	DRIVER( royalcdb )	/* (c) 1991 TAB-Austria */
	DRIVER( magiccrd )	/* (c) 1996 Impera */
	DRIVER( vroulet )	/* (c) 1989 World Game */
	DRIVER( rcasino )	/* (c) 1984 Dyna Electronics */
	DRIVER( cmv801 )	/* (c) 198? Corsica */
	DRIVER( cm2v841 )	/* (c) 198? Dyna Electronics */
	DRIVER( cm2841a )	/* (c) 198? Dyna Electronics */
	DRIVER( ampoker2 )	/* (c) 198? Novomatic */
	DRIVER( ampokr2a )	/* (c) 198? Novomatic */
	DRIVER( ampokr2b )	/* (c) 198? Novomatic */
	DRIVER( ampokr2c )	/* (c) 198? Novomatic */
	DRIVER( lucky8 )	/* (c) 1989 Falcon */
	DRIVER( starspnr )	/* (c) 1982? ACE */
	DRIVER( vp906iii )	/* (c) 1985 Casino Electronics Inc. */
	DRIVER( attckufo )	/* (c) 1980 Ryoto Electric Co. */
	DRIVER( fortecar )
	DRIVER( jackpool )	/* (c) 1997 Electronic Projects */
	DRIVER( murogem )	/* ??? */
	DRIVER( quizmstr )	/* (c) 1985 Coinmaster */
	DRIVER( trailblz )	/* (c) 1987 Coinmaster */
	DRIVER( supnudg2 )	/* (c) 1989 Coinmaster */
	DRIVER( egghunt )	/* (c) 1995 Invi Image */
	DRIVER( magic10 )	/* (c) 1995 A.W.P. Games */
	DRIVER( magic10a )	/* (c) 1995 A.W.P. Games */
	DRIVER( magic102 )	/* (c) 1997 ABM Games */
	DRIVER( dwarfd )	/* (c) 198? Electro-Sports */
	DRIVER( trvquest )	/* (c) 1984 Sunn / Techstar */
	DRIVER( wheelfir )	/* (c) 199? TCH */
	DRIVER( littlerb )	/* (c) 1993 TCH */
	DRIVER( tattack )	/* (c) 198? Shonan */
	DRIVER( mosaicf2 )	/* (c) 1999 F2 System */
	DRIVER( rbmk )		/* (c) 1995 GMS */
	DRIVER( go2000 )	/* (c) 2000 SA */
	DRIVER( mirax )		/* (c) 1985 Current Technologies */
	DRIVER( lastfght )	/* (c) 2000 Subsino */
	DRIVER( sliver )	/* (c) 1996 Hollow Corp */
	DRIVER( intrscti )	/* ???? */

#endif	/* DRIVER_RECURSIVE */
