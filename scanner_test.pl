#!/usr/bin/perl

use strict;
use warnings;

my $program_to_run       = "./your_program.sh";
my $test_file_input      = "test.lox";
my $expected_output_file = "expected_output.txt"; # Must contain build log + marker + token stream
my $latest_output_file   = "latest_output.txt";   # Will contain build log + marker + token stream

my $build_marker         = "[100%] Built target interpreter";

# --- Subroutine to open a file and find the marker ---
# Returns: filehandle positioned after the marker, or undef if marker not found
sub open_and_skip_to_after_marker {
    my ($filename, $marker) = @_;
    open(my $fh, '<', $filename) or die "ERROR: Cannot open $filename: $!\n";
    my $marker_found = 0;
    while (my $line = <$fh>) {
        if ($line =~ /\Q$marker\E/) { # \Q \E for literal match of marker
            $marker_found = 1;
            last; # Marker found, loop will exit, $fh is positioned after marker line
        }
    }
    unless ($marker_found) {
        close($fh);
        return undef;
    }
    return $fh; # Return filehandle positioned after the marker
}

# --- Main Script ---

# 1. Ensure prerequisite files exist
unless (-f $test_file_input) {
    print "ERROR: Test input file '$test_file_input' not found.\n";
    exit 1;
}
unless (-f $expected_output_file) {
    print "ERROR: Expected output file '$expected_output_file' not found.\n";
    print "       It must be the full output from a known-good run, including the line: \"$build_marker\"\n";
    exit 1;
}

# 2. Run your program. Overwrite $latest_output_file. Capture STDOUT and STDERR.
print "Running: $program_to_run tokenize $test_file_input ...\n";
system("$program_to_run tokenize \"$test_file_input\" > \"$latest_output_file\" 2>&1");

# 3. Open files and position after the marker
my $fh_latest = open_and_skip_to_after_marker($latest_output_file, $build_marker);
unless ($fh_latest) {
    print "TEST FAILED: Build marker \"$build_marker\" NOT found in '$latest_output_file'. Build may have failed.\n";
    unlink $latest_output_file; # Clean up
    exit 1;
}

my $fh_expected = open_and_skip_to_after_marker($expected_output_file, $build_marker);
unless ($fh_expected) {
    print "TEST SETUP ERROR: Build marker \"$build_marker\" NOT found in '$expected_output_file'. Please regenerate it correctly.\n";
    close($fh_latest);
    unlink $latest_output_file; # Clean up
    exit 1;
}

# 4. Compare line by line from this point
my $line_number = 0;
my $test_passed = 1;

while (1) {
    my $latest_line   = <$fh_latest>;
    my $expected_line = <$fh_expected>;
    $line_number++;

    # Check if both files ended at the same time
    if (!defined $latest_line && !defined $expected_line) {
        last; # Both EOF, good.
    }

    # Check if one file ended before the other
    if (!defined $latest_line) {
        print "Test failed: Actual output ended prematurely at line $line_number (after marker).\n";
        print "Expected more lines like: $expected_line";
        $test_passed = 0;
        last;
    }
    if (!defined $expected_line) {
        print "Test failed: Expected output ended prematurely at line $line_number (after marker).\n";
        print "Actual output has more lines like: $latest_line";
        $test_passed = 0;
        last;
    }

    # Compare the lines
    if ($latest_line ne $expected_line) {
        print "Test failed: Difference at line $line_number (after marker):\n";
        print "Expected: $expected_line";
        print "Actual:   $latest_line";
        $test_passed = 0;
        last; # Report only the first differing line
    }
}

close($fh_latest);
close($fh_expected);
unlink $latest_output_file; # Clean up

# 5. Final Report
if ($test_passed) {
    print "Test passed.\n";
    exit 0;
} else {
    # Specific error already printed
    exit 1;
}
