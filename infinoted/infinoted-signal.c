/* libinfinity - a GObject-based infinote implementation
 * Copyright (C) 2007, 2008, 2009 Armin Burgmeier <armin@arbur.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <infinoted/infinoted-signal.h>
#include <libinfinity/inf-i18n.h>

#ifdef LIBINFINITY_HAVE_LIBDAEMON
#include <libdaemon/dsignal.h>
#endif

#ifdef G_OS_WIN32
# include <windows.h>
#endif

#ifdef LIBINFINITY_HAVE_LIBDAEMON
static void
infinoted_signal_sig_func(InfNativeSocket* fd,
                          InfIoEvent event,
                          gpointer user_data)
{
  InfinotedSignal* sig;
  int occured;

  sig = (InfinotedSignal*)user_data;

  if(event & INF_IO_ERROR)
  {
    inf_io_watch(INF_IO(sig->run->io), &sig->signal_fd, 0, NULL, NULL, NULL);
    daemon_signal_done();

    sig->run = NULL;
    sig->signal_fd = 0;

    fprintf(stderr, "%s\n", _("Error on signal handler connection; signal "
                              "handlers have been removed from now on"));
  }
  else if(event & INF_IO_INCOMING)
  {
    occured = daemon_signal_next();
    if(occured == SIGINT || occured == SIGTERM || occured == SIGQUIT)
    {
      printf("\n");
      inf_standalone_io_loop_quit(sig->run->io);
    }
    else if(occured == SIGHUP)
    {
      fprintf(stderr, "%s\n", _("Config file reloading has not yet "
                                "been implemented"));
    }
  }
}
#else
static InfinotedRun* _infinoted_signal_server = NULL;

static void
infinoted_signal_terminate(void)
{
	InfinotedRun* run;

  /* We do a hard exit here, not calling inf_standalone_io_loop_quit(),
   * because the signal handler could be called from anywhere in the code. */
	if(_infinoted_signal_server != NULL)
	{
		run = _infinoted_signal_server;
		_infinoted_signal_server = NULL;

		infinoted_run_free(run);

		exit(0);
	}
}

static void
infinoted_signal_sigint_handler(int sig)
{
  printf("\n");
  infinoted_signal_terminate();
}

static void
infinoted_signal_sigterm_handler(int sig)
{
  printf("\n");
  infinoted_signal_terminate();
}

static void
infinoted_signal_sigquit_handler(int sig)
{
  printf("\n");
  infinoted_signal_terminate();
}

static void
infinoted_signal_sighup_handler(int sig)
{
  /* We don't reload the config file here since the signal handler could be
   * called from anywhere in the code. */
  fprintf(stderr, "%s\n", _("For config reloading to work libinfinity needs "
                            "to be compiled with libdaemon support"));

  /* Make sure the signal handler is not reset */
  signal(SIGHUP, infinoted_signal_sighup_handler);
}
#endif

#ifdef G_OS_WIN32
BOOL WINAPI infinoted_signal_console_handler(DWORD fdwCtrlType)
{
  /* TODO: Don't terminate for CTRL_LOGOFF_EVENT? */
  infinoted_signal_terminate();
  /* Doesn't matter, we exit() anyway */
  return TRUE;
}
#endif

/**
 * infinoted_signal_register:
 * @run: A #InfinotedRun.
 *
 * Registers signal handlers for SIGINT and SIGTERM that terminate the given
 * infinote server. When you don't need the signal handlers anymore, you
 * must unregister them again using infinoted_signal_unregister().
 *
 * Returns: A #InfinotedSignal to unregister the signal handlers again later.
 */
InfinotedSignal*
infinoted_signal_register(InfinotedRun* run)
{
  InfinotedSignal* sig;
  sig = g_slice_new(InfinotedSignal);

#ifdef LIBINFINITY_HAVE_LIBDAEMON
  sig->run = run;

  /* TODO: Should we report when this fails? Should ideally happen before
   * actually forking then - are signal connections kept in fork()'s child? */
  if(daemon_signal_init(SIGINT, SIGTERM, SIGQUIT, SIGHUP, 0) == 0)
  {
    sig->signal_fd = daemon_signal_fd();

    inf_io_watch(
      INF_IO(run->io),
      &sig->signal_fd,
      INF_IO_INCOMING | INF_IO_ERROR,
      infinoted_signal_sig_func,
      sig,
      NULL
    );
  }

#else
  sig->previous_sigint_handler =
    signal(SIGINT, &infinoted_signal_sigint_handler);
  sig->previous_sigterm_handler =
    signal(SIGTERM, &infinoted_signal_sigterm_handler);
  sig->previous_sigquit_handler =
    signal(SIGQUIT, &infinoted_signal_sigquit_handler);
  sig->previous_sighup_handler =
    signal(SIGHUP, &infinoted_signal_sighup_handler);

  _infinoted_signal_server = run;
#endif

#ifdef G_OS_WIN32
  SetConsoleCtrlHandler(infinoted_signal_console_handler, TRUE);
#endif

  return sig;
}

/**
 * infinoted_signal_unregister:
 * @sig: A #InfinotedSignal.
 *
 * Unregisters signal handlers registered with infinoted_signal_register().
 */
void
infinoted_signal_unregister(InfinotedSignal* sig)
{
#ifdef G_OS_WIN32
  SetConsoleCtrlHandler(infinoted_signal_console_handler, FALSE);
#endif

#ifdef LIBINFINITY_HAVE_LIBDAEMON
  if(sig->run)
  {
    inf_io_watch(INF_IO(sig->run->io), &sig->signal_fd, 0, NULL, NULL, NULL);
    daemon_signal_done();
  }
#else
  signal(SIGINT, sig->previous_sigint_handler);
  signal(SIGTERM, sig->previous_sigterm_handler);
  signal(SIGQUIT, sig->previous_sigquit_handler);
  signal(SIGHUP, sig->previous_sighup_handler);

  _infinoted_signal_server = NULL;
#endif

  g_slice_free(InfinotedSignal, sig);
}

/* vim:set et sw=2 ts=2: */
