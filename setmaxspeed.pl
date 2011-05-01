#!/usr/bin/perl
#*******************************************************************************
#    This file is part of spectro
#    Copyright (C) 2011  Amithash Prasad <amithash@gmail.com>
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

my @avail_freqs = split(/\s+/, `cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies`);
my $max_freq = 0;
foreach my $freq (@avail_freqs) {
	$freq = $freq + 0;
	if($max_freq < $freq) {
		$max_freq = $freq;
	}
}

my $nr_cpus = 0;

foreach my $line (split(/\n/, `cat /proc/cpuinfo | grep "cpu MHz"`)) {
	$nr_cpus++;
}
print "Num Cpus = $nr_cpus\n";

my $max_freq_mhz = $max_freq / 1000.0;

if($max_freq == 0 or $nr_cpus == 0) {
	printf("Error!\n");
	exit;
}

while(1) {
	my @freqs = split(/\n/, `cat /proc/cpuinfo | grep "cpu MHz"`);
	my $min_cpu_freq = $max_freq_mhz;
	foreach my $freq (@freqs) {
		if($freq =~ /^cpu MHz\s+:\s+(\d+\.\d+)\s*$/) {
			$freq = $1 + 0;
		} else {
			print "Regex Error!\n";
			 next;
		}
		if($min_cpu_freq > $freq) {
			$min_cpu_freq = $freq;
		}
	}
	if($min_cpu_freq < $max_freq_mhz) {
		print "Speed has reduced to $min_cpu_freq MHz, resetting it\n";
		SetSpeed($max_freq);
#		system("./setspeed.sh");
	}
	sleep(2);
}

sub SetSpeed
{
	my $speed_khz = shift;
	for(my $i = 0; $i < $nr_cpus; $i++) {
		system("echo \"performance\" > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor");
		system("echo $speed_khz > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_max_freq");
		system("echo $speed_khz > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq");
	}
}
