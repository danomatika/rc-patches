use POSIX;

$min = $ARGV[0]*1000;
$max = $ARGV[1]*1000;

%others = ();
$speed = 1;
$time = 0;
$compensated = 0;
while(<stdin>)
{ 	
	# store for compensation later
	m|^([\d\.e\-]+)(.* [/a-z]+)( .*)$|;
	$time += $1;
	$key = $2;
	$rest = $3;
	if ( $key ne " osc-playback send /tron/looper/nextsamp" )
	{
		$others{ $key }=$rest;
	}
#	print "$time $min $max\n";
	if ( $time > $min && $time < $max )
	{
		if ( $compensated==0 )
		{
			# compensate
			while ( my ($key, $value) = each(%others) ) {
	        		print "0.0".$key.$value."\n";
			}
			print (sprintf("%.3f",$time-$min)." dummy-ignore;\n");
			$compensated = 1;
		}
		print; 
	}
}

$min /= 1000;
$max /= 1000;
$min_minutes = floor($min/60);
$min_seconds = $min - ($min_minutes*60);
$max_minutes = floor($max/60);
$max_seconds = $max - ($max_minutes*60);

print STDERR "\nnow go mp3splt <filename> $min_minutes.$min_seconds $max_minutes.$max_seconds\n";
