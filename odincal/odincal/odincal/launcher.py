""" A launcher to launch jobs"""
import os
import subprocess
import tempfile
import sys
import logging


class ShellLauncher(object):  # pylint: disable=too-few-public-methods
    """A Launcher that runs at local host"""

    def submit(self, *args, **kwargs):  # pylint: disable=no-self-use
        """submitfunction
        """
        name = kwargs['Job_Name']
        outfile = kwargs['Output_Path']
        errfile = kwargs['Error_Path']
        logging.info('job: "%s" out: "%s" err: "%s"', name, outfile, errfile)
        newpid = os.fork()
        if newpid == 0:
            # this is the child process that now runs in the backgroud
            err = file(
                '{2}/{1}_{0}.er'.format(os.getpid(), name, errfile), 'w')
            out = file(
                '{2}/{1}_{0}.ou'.format(os.getpid(), name, outfile), 'w')
            python = subprocess.Popen(
                'python ' + args[0], shell=True, stdout=out, stderr=err
            )
            python.wait()
            err.close()
            out.close()
            sys.exit(0)
            # child is dead
        else:
            # returns the pid of the child
            return newpid


def main():
    """the main process"""
    launcher = ShellLauncher()
    # start two processings at the same time
    for _ in range(2):
        # create the batch script
        tmpfile = tempfile.NamedTemporaryFile(delete=False)
        tmpfile.write('sleep 10\necho "hello wold"\n')
        tmpfile.flush()
        tmpfile.close()
        # send the job, same syntax as with Torque.
        launcher.submit(
            tmpfile.name, 'gosat',
            Job_Name='preprocess',
            Variable_List='PGHOST=smiles-p12',
            Error_Path='./',
            Output_Path='./',
        )


if __name__ == '__main__':
    main()
