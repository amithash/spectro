#!/usr/bin/perl
#*******************************************************************************
#    This file is part of spectro
#    Copyright (C) 2010  Amithash Prasad <amithash@gmail.com>
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#*******************************************************************************

use strict;
use warnings;
use File::Find;
use File::Basename;
use threads ('yield',
'stack_size' => 64*4096,
'exit' => 'threads_only',
'stringify');
use threads::shared;

if(scalar(@ARGV) < 1) {
	print "USAGE: $0 <MusicDir> [Optional HistDB Name]\n";
	exit;
}
my $MusicDir = $ARGV[0];
unless(-d $MusicDir) {
	print "$MusicDir does not exist or is not a directory\n";
	exit;
}

unless(SpectAppsExist()) {
	print "Spect apps do not exist. Please perform 'make install' in spectro dir as root\n";
	exit;
}
my $cpus = NumCpus();
my $io_bound_threads = $cpus * 2;
my $cpu_bound_threads = $cpus;

if($MusicDir =~ /^(.+)\/+$/) {
	$MusicDir = $1;
}
my $SList = "$MusicDir.slist";
my $SDB = "$MusicDir.sdb";
my $HDB = "$MusicDir.hdb";
if(scalar(@ARGV >= 2)) {
	$HDB = $ARGV[1];
}
# First, Run spectgen
chdir $MusicDir;
my @MP3Files;
my $start_time;
my $end_time;

my $total_start_time = time;

###############################################################################
#		             SPECTGEN SECTION				      #
###############################################################################
print "#################################################################################\n";
print "Analyzing your MP3 files.\n";
print "This process is I/O intensive (Reading and analyzing each and every\n";
print "mp3 file can be a bit intensive) and can take up to 8-10 hours for 8000 mp3 files\n\n";
print "#################################################################################\n";
$start_time = time;
find sub {
	my $f = $File::Find::name;

	# XXX ADD Stuff here for ogg, etc support. Note, wma does not work that well.
	# If you have wma files on a linux box... god help you, surely microsoft wont.
	if($f =~ /\.[Mm][Pp]3$/) { 
		push @MP3Files, $f;
	}
}, ($MusicDir);

# Parallel this block
my @cmds;
for(my $i = 0; $i <= $#MP3Files; $i++) {
	my $c = spectgen($MP3Files[$i]);
	push @cmds, $c if($c ne "");
}
ExecuteCmdsParallel(\@cmds, $io_bound_threads);
$end_time = time;
PrintFormatTime("Generation of spect files time: ", $end_time - $start_time);

print "#################################################################################\n";
###############################################################################
#			      Create Spect List				      #
###############################################################################
system("find $MusicDir -name '*.spect' > $SList");

###############################################################################
#		        Generate Spect DB				      #
###############################################################################
print "#################################################################################\n";
print "\n\nCreating a single DB file from individual spect files. This usually\n";
print "Takes less than half hour\n";
print "#################################################################################\n";
$start_time = time;
system("genspectdb $SList $SDB") == 0 or die "Spect db generation failed";
unlink $SList; # Not needed anymore
$end_time = time;
PrintFormatTime("Spect DB generation time: ", $end_time - $start_time);
print "#################################################################################\n";
###############################################################################
#		     Normalize Spect DB	in place			      #
###############################################################################
print "#################################################################################\n";
print "\n\n Normalizing the spect db file (In place). This usually takes about an\n";
print "Hour to complete...\n";
print "#################################################################################\n";

$start_time = time;
system("normspectdb $SDB") == 0 or die "normspectdb failed!\n";
$end_time = time;
PrintFormatTime("Normalizing spect db time: ", $end_time - $start_time);

###############################################################################
#		        Generate Hist DB				      #
###############################################################################
print "#################################################################################\n";
print "\n\nCreating the HIST DB file. This usually take 10-20 minutes\n";
print "I have seen it get over in less than 10 minutes\n";
print "#################################################################################\n";
$start_time = time;
system("spect2hist $SDB $HDB 2> /dev/null") == 0 or die "spect2hist failed\n";
unlink $SDB;
$end_time = time;
PrintFormatTime("Hist DB generation time: ", $end_time - $start_time);

###############################################################################
#		        	END					      #
###############################################################################
print "#################################################################################\n";
print "\n\nFinished!";
print "#################################################################################\n";
my $total_end_time = time;
PrintFormatTime("Total execution Time: ", $total_end_time - $total_start_time);


print "Now, you can start spectradio, and C+F (Or File->Load DB)\n";
print "And point it to: \n";
print "$HDB\n";
print "\n\n	ENJOY\n\n";
print "#################################################################################\n";


sub spectgen
{
	my $mp3 = shift;
	my $base = basename($mp3);
	my $dir  = dirname($mp3);
	my $spect;
	if($base =~ /(.+)\.[Mm][Pp]3$/) {
		$spect = $1;
	} else {
		return;
	}
	$spect = "$dir/.$spect.spect";
	return "" if(-e $spect);
	$mp3   =~ s/"/\\"/g;
	$spect =~ s/"/\\"/g;
	return "spectgen \"$mp3\" -o \"$spect\" > /dev/null 2> /dev/null";
}

sub SpectAppsExist
{
	my @apps = ('spectgen', 'genspectdb', 'normspectdb', 'spect2hist');
	foreach my $app (@apps) {
		unless(AppExists($app)) {
			return 0;
		}
	}
	return 1;
}

sub AppExists
{
	my $app = shift;
	if(`which $app 2> /dev/null` =~ /^\s*$/) {
		return 0;
	}
	return 1;
}

sub NumCpus
{
	my $str = `cat /proc/cpuinfo`;
	my @tmp = split(/\n/,$str);
	my $cpus = 0;
	foreach my $t (@tmp) {
		chomp($t);
		if($t =~ /processor\s+:\s+\d/) {
			$cpus += 1;
		}
	}
	$cpus = 1 if($cpus == 0);
	return $cpus;
}

sub progress
{
	$|=1;
	my $percent = shift;

	if($percent < 0){
		$percent = 0;
	}
	if($percent > 100){
		$percent = 100;
	}
	if($percent == 0) {
		print "\r";
	}

        # Print Progress
	print "|";
	for(my $k=0;$k<$percent;$k=$k+5)
	{
		print "#";						
	}
	for(my $k=$percent;$k<100;$k=$k+5){
		print "-";
	}
	print "|";
	printf " <-- %.2f%% Complete --> ",$percent;

        # Goto the start of the line
	if($percent < 100){
		print "\r";
	} else {
		print "\n";
	}
}

sub ExecuteCmdsParallel
{
	my $cmd_ref = shift;
	my $threads :shared;
	$threads = shift;
	my @cmds :shared;
	my $total_failed :shared;
	my $len :shared;
	my $n :shared;
	my $i :shared;
	my $total_count : shared;

	$total_count = 0;
	@cmds = @{$cmd_ref};
	$total_failed = 0;
	$len = scalar(@cmds);

	if($len < 2 * $threads) {
		$threads = 1; # Do not complicate by parallel if job is small.
	}
	$n = int($len / $threads);
	my @thr;
	for($i = 0; $i < $threads; $i++) {
		my $tid = $i;
		$thr[$i] = async {
			my $start = $tid * $n;
			my $end = ($tid + 1) * $n;
			$end = $len if($tid == $threads - 1);
			$end = $end - 1;
			my @tmp = @cmds[$start...$end];
			foreach my $c (@tmp) {
				if(system($c) != 0) {
					lock($total_failed);
					$total_failed = $total_failed + 1;
				}
				lock($total_count);
				$total_count++;
			}
		};
	}
	my $progress_thr = async {
		while($total_count < $len) {
			progress(100 * $total_count/$len);
			sleep(1);
		}
		progress(100);
	};
	for(my $i = 0; $i < $threads; $i++) {
		$thr[$i]->join();
	}
	$progress_thr->join();
	if($total_failed > 0) {
		print "$total_failed/$len jobs failed.\n";
	}
}

sub PrintFormatTime
{
	my $txt = shift;
	my $sec = shift;
	my $min = 0;
	my $hour = 0;

	$min = int($sec / 60);
	$sec = $sec % 60;

	$hour = int($min / 60);
	$min = $min % 60;

	print "$txt $hour hours, $min minutes, $sec seconds\n";
}
