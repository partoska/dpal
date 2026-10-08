Daemon Pal (dpal)

Installed files:
  /usr/bin/dpal            The dpal command-line tool.

Quick start:
  dpal -C &                Start the control process (daemon).
  dpal -l -a               List all managed processes.
  dpal -k                  Shut down the control process.

Run "dpal -h" for full usage. The control process and its command
executors share a working directory (default: ${DPAL_HOME} or
${HOME}/.dpal); pass the same -D to both when using a custom location.

Daemon Pal is licensed under the MIT License.
