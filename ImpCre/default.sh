#!/bin/sh

# Imperium
#
# default.sh - Example shell script giving the default values for ImpCre
#
# Add the location of ImpCre to our path
PATH=$PATH:/usr/local/lib/imperium/impcre;export PATH

# Here are the various ENV variables you can set, along with their default
# values.

WorldFile=imp.world
PlanetsFile=imp.planet
SectorsFile=imp.sector
BigItemFile=imp.bigitem
PlayerFile=imp.player
ShipFile=imp.ship
FleetFile=imp.fleet
LoanFile=imp.loan
OfferFile=imp.offer
LogFile=imp.log

# Set this to the name of the team/player that won the last game
LastWinner=

# Set this to the deities password
GodPassword=imperium

# Set this to the password needed to create a new player
CreationPassword=add.me

# Here are the default race names along with the default name of their
# home planet.
Race1Name=Voltar
Race2Name=Human
Race3Name=Nakasumi
Race4Name=Krellian
Race5Name=Kindit
Race6Name=Ego
Race7Name=Dorn
Race8Name=Zyxylck
Race1Planet=Voltar
Race2Planet=Terra
Race3Planet=Tokiga
Race4Planet=Krell
Race5Planet=Targe
Race6Planet=Arcadia
Race7Planet=Huraza
Race8Planet=Krykcoq

# Set this to the random number seed, or 0 to have one picked for you
RandomSeed=0

# The horizontal size of the world in *GALACTIC* sectors. Must be >=10
HorizSize=10

# The vertical size of the world in *GALACTIC* sectors. Must be >=10
VertSize=10

# The percentage of galactic sectors that will be "other" sector types
PctOth=5

# The percentage of the above "other" sectors that will be black holes
PctBH=50

# The intended number of players for the game (including the deity)
NumPlay=6

# The maximum daily connect time allowed for players
ConTime=60

# How much time makes up an "Imperium Time Unit" in seconds
SecItu=1800

# How much money the players start out with
StartingCash=5000

MinPlSpc=3
MinPlAdj=3
MinUnclaimed=5
MaxReach=15
MaxSpread=15

# What percentage of the galactic sectors will have at least SOME stars
LowStarPct=95

# What percentage of the galactic sectors will have a HI humber of stars
# Must be <= LowStarPct
HiStarPct=25

# The minimum star size. Must be >= 3
MinStar=6

# The maximum star size. Must be <= 9
MaxStar=9

# What percentage of the galactic sectors will have at least SOME planets
LowPlPct=75

# What percentage of the galactic sectors will have a HI humber of planets
# Must be <= LowPlPct
HiPlPct=10

# The minimum planet size. Must be >= 1
MinPlan=1

# The maximum star size. Must be < MinStar
MaxPlan=5

# The size of the home planets. Must be >= MinPlan and <= MaxPlan
HomeSize=2

# Email address to send mail from/to
EmailAddr=changeme@foobar.com

# Boolean flags - set to "1" for true, and "0" for false

# Are players allowed to use the "change player" command?
ChangePlayers=1

# Are players allowed to send "public" messages (telegrams to a group
# of players via wildcards)
PublicMessages=1

# This is off by default so that it will ask you for
# confirmation before creating the world (if successfull)
# DoNotAsk=1;export DoNotAsk

# ---------------- Don't change from here down

export WorldFile PlanetsFile SectorsFile BigItemFile PlayerFile ShipFile \
	FleetFile LoanFile OfferFile LogFile LastWinner GodPassword \
	CreationPassword Race1Name Race2Name Race3Name Race4Name
export Race5Name Race6Name Race7Name Race8Name Race1Planet Race2Planet \
	Race3Planet Race4Planet Race5Planet Race6Planet Race7Planet \
	Race8Planet RandomSeed HorizSize VertSize PctOth PctBH
export NumPlay ConTime SecItu StartingCash MinPlSpc MinPlAdj MinUnclaimed \
	MaxReach MaxSpread LowStarPct HiStarPct MinStar MaxStar LowPlPct \
	HiPlPct MinPlan MaxPlan HomeSize ChangePlayers PublicMessages
export EmailAddr
exec ImpCre -v
