/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Günther
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

/* A wrapper class to OS dependent event and window interfaces, X11 version */

#include <Standard.h>

#ifdef USE_X11

#include <StdWindow.h>
#include <StdGL.h>
#include <StdDDraw2.h>
#include <StdFile.h>
#include <StdBuf.h>

#include <X11/Xmd.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/xf86vmode.h>
#include <X11/XKBlib.h>

#include <string>

#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

#ifdef HAVE_LIBREADLINE
#if defined(HAVE_READLINE_READLINE_H)
	#include <readline/readline.h>
#elif defined(HAVE_READLINE_H)
	#include <readline.h>
#endif
static void readline_callback(char *);
static CStdApp *readline_callback_use_this_app = nullptr;
#endif /* HAVE_LIBREADLINE */

#ifdef HAVE_READLINE_HISTORY
	#if defined(HAVE_READLINE_HISTORY_H)
		#include <readline/history.h>
	#elif defined(HAVE_HISTORY_H)
		#include <history.h>
	# endif
#endif /* HAVE_READLINE_HISTORY */

/* CStdApp */

#ifdef WITH_GLIB
#include <glib.h>
#endif

#include "StdXPrivate.h"

namespace
{
#ifdef WITH_GLIB
	// Callbacks from Glib main loop
	gboolean OnXInputStatic(GIOChannel *channel, GIOCondition condition, gpointer data)
	{
		static_cast<CStdApp *>(data)->OnXInput();
		return TRUE;
	}

	gboolean OnPipeInputStatic(GIOChannel *channel, GIOCondition condition, gpointer data)
	{
		static_cast<CStdApp *>(data)->OnPipeInput();
		return TRUE;
	}

	gboolean OnStdInInputStatic(GIOChannel *channel, GIOCondition condition, gpointer data)
	{
		static_cast<CStdApp *>(data)->OnStdInInput();
		return TRUE;
	}
#endif

	unsigned int KeyMaskFromKeyEvent(Display *dpy, XKeyEvent *xkey)
	{
		unsigned int mask = xkey->state;
		KeySym sym = XkbKeycodeToKeysym(dpy, xkey->keycode, 0, 1);
		// We need to correct the keymask since the event.xkey.state
		// is the state _before_ the event, but we want to store the
		// current state.
		if (sym == XK_Control_L || sym == XK_Control_R) mask ^= MK_CONTROL;
		if (sym == XK_Shift_L   || sym == XK_Shift_R)   mask ^= MK_SHIFT;
		if (sym == XK_Alt_L     || sym == XK_Alt_R)     mask ^= (1 << 3);
		return mask;
	}
}

CStdAppPrivate::WindowListT CStdAppPrivate::WindowList;

CStdApp::CStdApp() : Active(false), fQuitMsgReceived(false), dpy(nullptr), Priv(new CStdAppPrivate()),
	Location(""), DoNotDelay(false), mainThread(std::this_thread::get_id()),
	// 36 FPS
	Delay(27777) {}

CStdApp::~CStdApp()
{
	delete Priv;
}

static Atom ClipboardAtoms[1];

void CStdApp::Init(int argc, char *argv[])
{
	// Set locale
	setlocale(LC_ALL, "");
	setlocale(LC_NUMERIC, "C");

	// Clear XMODIFIERS as key input gets evaluated twice otherwise
	unsetenv("XMODIFIERS");

	// Try to figure out the location of the executable
	Priv->argc = argc; Priv->argv = argv;
	static char dir[PATH_MAX];
	SCopy(argv[0], dir);
	if (dir[0] != '/')
	{
		SInsert(dir, "/");
		SInsert(dir, GetWorkingDirectory());
		Location = dir;
	}
	else
	{
		Location = dir;
	}
	// botch arguments
	static std::string s("\"");
	for (int i = 1; i < argc; ++i)
	{
		s.append(argv[i]);
		s.append("\" \"");
	}
	s.append("\"");
	szCmdLine = s.c_str();

#ifdef WITH_GLIB
	Priv->loop = g_main_loop_new(nullptr, FALSE);

	Priv->pipe_channel = nullptr;
	Priv->x_channel = nullptr;
	Priv->stdin_channel = nullptr;
#endif

	if (!(dpy = XOpenDisplay(nullptr)))
	{
		throw StartupException{"Error opening display."};
	}

	int xf86vmode_event_base, xf86vmode_error_base;
	if (!XF86VidModeQueryExtension(dpy, &xf86vmode_event_base, &xf86vmode_error_base)
		|| !XF86VidModeQueryVersion(dpy, &xf86vmode_major_version, &xf86vmode_minor_version))
	{
		xf86vmode_major_version = -1;
		xf86vmode_minor_version = 0;
		Log("XF86VidMode Extension missing, resolution switching will not work");
	}

	// So a repeated keypress-event is not preceded with a keyrelease.
	XkbSetDetectableAutoRepeat(dpy, True, &Priv->detectable_autorepeat_supported);

	XSetLocaleModifiers("");
	Priv->xim = XOpenIM(dpy, nullptr, nullptr, nullptr);
	if (!Priv->xim) Log("Failed to open input method.");

	// Get the Atoms for the Clipboard
	const char *names[] = { "CLIPBOARD" };
	XInternAtoms(dpy, const_cast<char **>(names), 1, false, ClipboardAtoms);

#ifdef WITH_GLIB
	Priv->x_channel = g_io_channel_unix_new(XConnectionNumber(dpy));
	g_io_add_watch(Priv->x_channel, G_IO_IN, &OnXInputStatic, this);
#endif
#if USE_CONSOLE && HAVE_LIBREADLINE
	rl_callback_handler_install(">", readline_callback);
	readline_callback_use_this_app = this;

#ifdef WITH_GLIB
	Priv->stdin_channel = g_io_channel_unix_new(STDIN_FILENO);
	g_io_add_watch(Priv->stdin_channel, G_IO_IN, &OnStdInInputStatic, this);
#endif
#endif
	// create pipe
	if (pipe(Priv->Pipe) != 0)
	{
		throw StartupException{"Error creating Pipe"};
	}

#ifdef WITH_GLIB
	Priv->pipe_channel = g_io_channel_unix_new(Priv->Pipe[0]);
	g_io_add_watch(Priv->pipe_channel, G_IO_IN, &OnPipeInputStatic, this);
#endif

	// Custom initialization
	DoInit();
}

bool CStdApp::InitTimer() { gettimeofday(&LastExecute, nullptr); return true; }

void CStdApp::Clear()
{
	if (dpy != nullptr)
	{
		XCloseDisplay(dpy);
		dpy = nullptr;
	}
#if USE_CONSOLE && HAVE_LIBREADLINE
	rl_callback_handler_remove();
#endif
	// close pipe
	close(Priv->Pipe[0]);
	close(Priv->Pipe[1]);
#ifdef WITH_GLIB
	g_main_loop_unref(Priv->loop);

	if (Priv->pipe_channel) g_io_channel_unref(Priv->pipe_channel);
	if (Priv->x_channel) g_io_channel_unref(Priv->x_channel);
	if (Priv->stdin_channel) g_io_channel_unref(Priv->stdin_channel);
#endif
}

void CStdApp::Quit()
{
	fQuitMsgReceived = true;
}

void CStdApp::Execute()
{
	time_t seconds = LastExecute.tv_sec;
	timeval tv;
	gettimeofday(&tv, nullptr);
	// Too slow?
	if (DoNotDelay)
	{
		DoNotDelay = false;
		LastExecute = tv;
	}
	else if (LastExecute.tv_sec < tv.tv_sec - 2)
	{
		LastExecute = tv;
	}
	else
	{
		LastExecute.tv_usec += Delay;
		if (LastExecute.tv_usec > 1000000)
		{
			++LastExecute.tv_sec;
			LastExecute.tv_usec -= 1000000;
		}
	}
	// This will make the FPS look "prettier" in some situations
	// But who cares...
	if (seconds != LastExecute.tv_sec)
	{
		pWindow->Sec1Timer();
	}
}

void CStdApp::NextTick(bool fYield)
{
	DoNotDelay = true;
}

void CStdApp::Run()
{
	// Main message loop
	while (true) if (HandleMessage(INFINITE, true) == HR_Failure) return;
}

void CStdApp::ResetTimer(unsigned int d) { Delay = 1000 * d; }

#ifdef WITH_GLIB
namespace
{
	// Just indicate that the timeout elapsed
	gboolean HandleMessageTimeout(gpointer data) { *static_cast<bool *>(data) = true; return FALSE; }
}
#endif

C4AppHandleResult CStdApp::HandleMessage(unsigned int iTimeout, bool fCheckTimer)
{
	// quit check for nested HandleMessage-calls
	if (fQuitMsgReceived) return HR_Failure;
	bool do_execute = fCheckTimer;
	// Wait Delay microseconds.
	timeval tv = { 0, 0 };
	if (DoNotDelay)
	{
		// nothing to do
	}
	else if (fCheckTimer)
	{
		gettimeofday(&tv, nullptr);
		tv.tv_usec = LastExecute.tv_usec - tv.tv_usec + Delay
			- 1000000 * (tv.tv_sec - LastExecute.tv_sec);
		// Check if the given timeout comes first
		// (don't call Execute then, because it assumes it has been called because of a timer event!)
		if (iTimeout != INFINITE && iTimeout * 1000 < tv.tv_usec)
		{
			tv.tv_usec = iTimeout * 1000;
			do_execute = false;
		}
		if (tv.tv_usec < 0)
			tv.tv_usec = 0;
		tv.tv_sec = 0;
	}
	else
	{
		tv.tv_usec = iTimeout * 1000;
	}

	// Handle pending X messages
	while (XPending(dpy))
	{
		HandleXMessage();
	}

#ifdef WITH_GLIB
	// Timeout in milliseconds
	unsigned int timeout = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	bool timeout_elapsed = false;
	guint timeout_handle = 0;

	// Guarantee that we do not block until something interesting occurs
	// when using a timeout
	if (fCheckTimer || iTimeout != INFINITE)
	{
		// The timeout handler sets timeout_elapsed to true when
		// the timeout elpased, this is required for a correct return
		// value.
		timeout_handle = g_timeout_add_full(
			G_PRIORITY_HIGH,
			timeout,
			&HandleMessageTimeout,
			&timeout_elapsed,
			nullptr
		);
	}

	g_main_context_iteration(g_main_loop_get_context(Priv->loop), TRUE);

	if (timeout_handle && !timeout_elapsed)
	{
		// FIXME: do not add a new timeout instead of deleting the old one in the next call
		g_source_remove(timeout_handle);
	}

	if (timeout_elapsed && do_execute)
	{
		Execute();
	}
	while (g_main_context_pending(g_main_loop_get_context(Priv->loop)))
		g_main_context_iteration(g_main_loop_get_context(Priv->loop), FALSE);

	return timeout_elapsed ? (do_execute ? HR_Timer : HR_Timeout) : HR_Message;
#else // WITH_GLIB
	// Watch dpy to see when it has input.
	int max_fd = 0;
	fd_set rfds;
	FD_ZERO(&rfds);

	// Stop waiting for the next frame when more events arrive
	XFlush(dpy);
	FD_SET(XConnectionNumber(dpy), &rfds);
	max_fd = (std::max)(XConnectionNumber(dpy), max_fd);
#ifdef USE_CONSOLE
	// Wait for commands from stdin
	FD_SET(0, &rfds);
#endif
	// And for events from the network thread
	FD_SET(Priv->Pipe[0], &rfds);
	max_fd = (std::max)(Priv->Pipe[0], max_fd);
	switch (select(max_fd + 1, &rfds, nullptr, nullptr, (fCheckTimer || iTimeout != INFINITE) ? &tv : nullptr))
	{
	// error
	case -1:
		if (errno == EINTR) return HR_Message;
		Log(strerror(errno));
		Log("select error.");
		return HR_Failure;

	// timeout
	case 0:
		if (do_execute)
		{
			Execute();
			return HR_Timer;
		}
		return HR_Timeout;

	default:
		// flush pipe
		if (FD_ISSET(Priv->Pipe[0], &rfds))
		{
			OnPipeInput();
		}
		if (FD_ISSET(XConnectionNumber(dpy), &rfds))
		{
			OnXInput();
		}
#ifdef USE_CONSOLE
		// handle commands
		if (FD_ISSET(0, &rfds))
		{
			// Do not call OnStdInInput to be able to return
			// HR_Failure when ReadStdInCommand returns false
			if (!ReadStdInCommand())
				return HR_Failure;
		}
#endif
		return HR_Message;
	}
#endif // !WITH_GLIB
}

bool CStdApp::SignalNetworkEvent()
{
	char c = 1;
	write(Priv->Pipe[1], &c, 1);
	return true;
}

void CStdApp::HandleXMessage()
{
	XEvent event;
	XNextEvent(dpy, &event);
	bool filtered = XFilterEvent(&event, event.xany.window);
	switch (event.type)
	{
	case FocusIn:
		if (Priv->xic) XSetICFocus(Priv->xic);
		break;
	case FocusOut:
		if (Priv->xic) XUnsetICFocus(Priv->xic);
		break;
	case EnterNotify:
		KeyMask = event.xcrossing.state;
		break;
	case KeyPress:
		// Needed for input methods
		if (!filtered)
		{
			KeyMask = KeyMaskFromKeyEvent(dpy, &event.xkey);
			char c[10] = "";
			if (Priv->xic)
			{
				Status lsret;
				XmbLookupString(Priv->xic, &event.xkey, c, 10, nullptr, &lsret);
				if (lsret == XLookupKeySym) fprintf(stderr, "FIXME: XmbLookupString returned XLookupKeySym\n");
				if (lsret == XBufferOverflow) fprintf(stderr, "FIXME: XmbLookupString returned XBufferOverflow\n");
			}
			else
			{
				static XComposeStatus state;
				XLookupString(&event.xkey, c, 10, nullptr, &state);
			}
			if (c[0])
			{
				CStdWindow *pWindow = Priv->GetWindow(event.xany.window);
				if (pWindow && !IsAltDown())
				{
					pWindow->CharIn(c);
				}
			}
			// Fallthrough
		}
	case KeyRelease:
		KeyMask = KeyMaskFromKeyEvent(dpy, &event.xkey);
		Priv->LastEventTime = event.xkey.time;
		break;
	case ButtonPress:
		// We can take this directly since there are no key presses
		// involved. TODO: We probably need to correct button state
		// here though.
		KeyMask = event.xbutton.state;
		Priv->LastEventTime = event.xbutton.time;
		break;
	case SelectionRequest:
	{
		// We should compare the timestamp with the timespan when we owned the selection
		// But slow network connections are not supported anyway, so do not bother
		CStdAppPrivate::ClipboardData &d = (event.xselectionrequest.selection == XA_PRIMARY) ?
			Priv->PrimarySelection : Priv->ClipboardSelection;
		XEvent responseevent;
		XSelectionEvent &re = responseevent.xselection;
		re.type = SelectionNotify;
		re.display = dpy;
		re.selection = event.xselectionrequest.selection;
		re.target = event.xselectionrequest.target;
		re.time = event.xselectionrequest.time;
		re.requestor = event.xselectionrequest.requestor;
		// Note: we're implementing the spec only partially here
		if (!d.Text.empty())
		{
			re.property = event.xselectionrequest.property;
			XChangeProperty(dpy, re.requestor, re.property, re.target, 8, PropModeReplace,
			    reinterpret_cast<const unsigned char *>(d.Text.c_str()), d.Text.size());
		}
		else
		{
			re.property = None;
		}
		XSendEvent(dpy, re.requestor, false, NoEventMask, &responseevent);
		break;
	}
	case SelectionClear:
	{
		CStdAppPrivate::ClipboardData &d = (event.xselectionrequest.selection == XA_PRIMARY) ?
			Priv->PrimarySelection : Priv->ClipboardSelection;
		d.Text.clear();
		break;
	}
	case ClientMessage:
		if (!strcmp(XGetAtomName(dpy, event.xclient.message_type), "WM_PROTOCOLS"))
		{
			if (!strcmp(XGetAtomName(dpy, event.xclient.data.l[0]), "WM_DELETE_WINDOW"))
			{
				CStdWindow *pWindow = Priv->GetWindow(event.xclient.window);
				if (pWindow) pWindow->Close();
			}
			else if (!strcmp(XGetAtomName(dpy, event.xclient.data.l[0]), "_NET_WM_PING"))
			{
				// We're still alive
				event.xclient.window = DefaultRootWindow(dpy);
				XSendEvent(dpy, DefaultRootWindow(dpy), false,
					SubstructureNotifyMask | SubstructureRedirectMask, &event);
			}
		}
		break;
	case MappingNotify:
		XRefreshKeyboardMapping(&event.xmapping);
		break;
	case DestroyNotify:
	{
		CStdWindow *pWindow = Priv->GetWindow(event.xany.window);
		if (pWindow)
		{
			pWindow->wnd = 0;
			pWindow->Clear();
		}
		Priv->SetWindow(event.xany.window, nullptr);
		break;
	}
	}
	CStdWindow *pWindow = Priv->GetWindow(event.xany.window);
	if (pWindow)
		pWindow->HandleMessage(event);
}

// Copy the text to the clipboard or the primary selection
bool CStdApp::Copy(std::string_view text, bool fClipboard)
{
	CStdAppPrivate::ClipboardData &d = fClipboard ? Priv->ClipboardSelection : Priv->PrimarySelection;
	XSetSelectionOwner(dpy, fClipboard ? ClipboardAtoms[0] : XA_PRIMARY, pWindow->wnd, Priv->LastEventTime);
	Window owner = XGetSelectionOwner(dpy, fClipboard ? ClipboardAtoms[0] : XA_PRIMARY);
	if (owner != pWindow->wnd) return false;
	d.Text = text;
	return true;
}

// Paste the text from the clipboard or the primary selection
std::string CStdApp::Paste(bool fClipboard)
{
	Window owner = XGetSelectionOwner(dpy, fClipboard ? ClipboardAtoms[0] : XA_PRIMARY);
	if (owner == None) return "";
	// Retrieve the selection into the XA_STRING property of our main window
	XConvertSelection(dpy, fClipboard ? ClipboardAtoms[0] : XA_PRIMARY, XA_STRING, XA_STRING,
		pWindow->wnd, Priv->LastEventTime);
	// Give the owner some time to respond
	HandleMessage(50, false);
	// Get the length of the data, so we can request it all at once
	Atom type;
	int format;
	unsigned long len, bytes_left;
	unsigned char *data;
	XGetWindowProperty(dpy, pWindow->wnd,
		XA_STRING,         // property
		0, 0,              // offset - len
		0,                 // do not delete it now
		AnyPropertyType,   // flag
		&type,             // return type
		&format,           // return format
		&len, &bytes_left, // that
		&data);
	// nothing to read?
	if (bytes_left == 0) return "";
	int result = XGetWindowProperty(dpy, pWindow->wnd,
		XA_STRING, 0, bytes_left,
		1, // delete it now
		AnyPropertyType,
		&type, &format, &len, &bytes_left, &data);
	if (result != Success) return "";
	std::string res{reinterpret_cast<char *>(data)};
	XFree(data);
	return res;
}

// Is there something in the clipboard?
bool CStdApp::IsClipboardFull(bool fClipboard)
{
	return None != XGetSelectionOwner(dpy, fClipboard ? ClipboardAtoms[0] : XA_PRIMARY);
}

CStdWindow *CStdAppPrivate::GetWindow(unsigned long wnd)
{
	WindowListT::iterator i = WindowList.find(wnd);
	if (i != WindowList.end()) return i->second;
	return nullptr;
}

void CStdAppPrivate::SetWindow(unsigned long wnd, CStdWindow *pWindow)
{
	if (!pWindow)
	{
		WindowList.erase(wnd);
	}
	else
	{
		WindowList[wnd] = pWindow;
	}
}

bool CStdApp::ReadStdInCommand()
{
#if HAVE_LIBREADLINE
	rl_callback_read_char();
	return true;
#else
	// Surely not the most efficient way to do it, but we won't have to read much data anyway.
	char c;
	if (read(0, &c, 1) != 1)
		return false;
	if (c == '\n')
	{
		if (!CmdBuf.isNull())
		{
			OnCommand(CmdBuf.getData()); CmdBuf.Clear();
		}
	}
	else if (isprint(static_cast<unsigned char>(c)))
		CmdBuf.AppendChar(c);
	return true;
#endif
}

#if HAVE_LIBREADLINE
static void readline_callback(char *line)
{
	if (!line)
	{
		readline_callback_use_this_app->Quit();
	}
	else
	{
		readline_callback_use_this_app->OnCommand(line);
	}
#if HAVE_READLINE_HISTORY
	if (line && *line)
	{
		add_history(line);
	}
#endif
	free(line);
}
#endif

void CStdApp::OnXInput()
{
	while (XEventsQueued(dpy, QueuedAfterReading))
	{
		HandleXMessage();
	}
}

void CStdApp::OnPipeInput()
{
	char c;
	::read(Priv->Pipe[0], &c, 1);
	// call network class to handle it
	OnNetworkEvents();
}

void CStdApp::OnStdInInput()
{
	if (!ReadStdInCommand())
	{
		// TODO: This should only cause HandleMessage to return
		// HR_Failure...
		Quit();
	}
}

#endif /* USE_X11 */
