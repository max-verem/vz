package VEREM::vz_commands;

use strict;
use vars qw(@ISA @EXPORT @EXPORT_OK %EXPORT_TAGS $VERSION %ENV);
use Exporter;
$VERSION = 1.00;
@ISA = qw(Exporter);
@EXPORT = qw();

use Data::Dump qw(dump);

sub send
{
    my @CMD = ();


    my ($host,$cmds) = @_;

    foreach my $cmd (@{$cmds})
    {
        if(defined($cmd->{SET_TEXT}))
        {
            push(@CMD, 'SET', $cmd->{CONTAINER}, 's_text', $cmd->{SET_TEXT});
        }
        elsif(defined($cmd->{SET_IMAGE}))
        {
            push(@CMD, 'SET', $cmd->{CONTAINER}, 's_filename', $cmd->{SET_IMAGE});
        }
        elsif(defined($cmd->{SET_ACTIVE}))
        {
            push(@CMD, 'CONTAINER_VISIBLE', $cmd->{CONTAINER}, $cmd->{SET_ACTIVE});
        }
        elsif(defined($cmd->{DIRECTOR}))
        {
            push(@CMD, 'START_DIRECTOR', $cmd->{DIRECTOR}, '0')
                if ($cmd->{BEHAVE} eq 'START');
            push(@CMD, 'START_DIRECTOR', $cmd->{DIRECTOR}, $cmd->{FRAME})
                if ($cmd->{BEHAVE} eq 'START_AT');
            push(@CMD, 'STOP_DIRECTOR', $cmd->{DIRECTOR})
                if ($cmd->{BEHAVE} eq 'STOP');
            push(@CMD, 'CONTINUE_DIRECTOR', $cmd->{DIRECTOR})
                if (($cmd->{BEHAVE} eq 'CONTINUE') || ($cmd->{BEHAVE} eq 'CONT'));
            push(@CMD, 'RESET_DIRECTOR', $cmd->{DIRECTOR}, '0')
                if ($cmd->{BEHAVE} eq 'RESET');
        }
        elsif(defined($cmd->{LAYER_LOAD}) && defined($cmd->{SCENE}))
        {
            push(@CMD, 'LAYER_LOAD', $cmd->{SCENE}, $cmd->{LAYER_LOAD});
        }
        elsif(defined($cmd->{LAYER_UNLOAD}))
        {
            push(@CMD, 'LAYER_UNLOAD', $cmd->{LAYER_UNLOAD});
        }
        elsif(defined($cmd->{SCENE}))
        {
            push(@CMD, 'LOAD_SCENE', $cmd->{SCENE});
        }
        elsif((defined($cmd->{FUNCTION}))&&(defined($cmd->{CONTAINER})))
        {
            push(@CMD, 'SET', $cmd->{CONTAINER}, $cmd->{FIELD}, $cmd->{VALUE});
        }
        elsif(defined($cmd->{SET}) && defined($cmd->{CONTAINER}) && defined($cmd->{VALUE}))
        {
            push(@CMD, 'SET', $cmd->{CONTAINER}, $cmd->{SET}, $cmd->{VALUE});
        };

    };


    # call prog to send command
    system('/var/www/users/vz/bin/vz_cmd_sender', $host, @CMD);

    if ($? == -1)
    {
        return "failed to execute: $!\n";
    }
    elsif ($? & 127)
    {
       return sprintf("child died with signal %d, %s coredump\n", ($? & 127),  ($? & 128) ? 'with' : 'without');
    };

    my $r = $? >> 8;

    if($r)
    {
        return sprintf("child exited with value %d\n", $r);
    }
    else
    {
        return undef;
    };
};

1;
