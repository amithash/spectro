#!/usr/bin/perl

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
