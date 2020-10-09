#include <gdk/gdkkeysyms-compat.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vte/vte.h>

static void set_font_size(VteTerminal *terminal, gint delta) {
  PangoFontDescription *descr;
  if ((descr = pango_font_description_copy(vte_terminal_get_font(terminal))) ==
      NULL)
    return;

  gint current = pango_font_description_get_size(descr);
  pango_font_description_set_size(descr, current + delta * PANGO_SCALE);
  vte_terminal_set_font(terminal, descr);
  pango_font_description_free(descr);
}

static void reset_font_size(VteTerminal *terminal) {
  PangoFontDescription *descr;
  if ((descr = pango_font_description_from_string("Comic Mono 14")) == NULL)
    return;
  vte_terminal_set_font(terminal, descr);
  pango_font_description_free(descr);
}

static gboolean on_char_size_changed(GtkWidget *terminal, guint width,
                                     guint height, gpointer user_data) {
  set_font_size(VTE_TERMINAL(terminal), 0);
  return TRUE;
}

static gboolean on_key_press(GtkWidget *terminal, GdkEventKey *event,
                             gpointer user_data) {
  switch (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_MOD1_MASK)) {
  case GDK_CONTROL_MASK | GDK_SHIFT_MASK:
    switch (event->keyval) {
    case GDK_KEY_V:
      vte_terminal_paste_clipboard(VTE_TERMINAL(terminal));
      return TRUE;
    }
  case GDK_CONTROL_MASK:
    switch (event->keyval) {
    case GDK_KEY_plus:
      set_font_size(VTE_TERMINAL(terminal), 1);
      return TRUE;
    case GDK_KEY_minus:
      set_font_size(VTE_TERMINAL(terminal), -1);
      return TRUE;
    case GDK_KEY_equal:
      reset_font_size(VTE_TERMINAL(terminal));
      return TRUE;
    case GDK_KEY_0:
      reset_font_size(VTE_TERMINAL(terminal));
      return TRUE;
    }
    break;
  }
  return FALSE;
}

static void child_ready(VteTerminal *terminal, GPid pid, GError *error,
                        gpointer user_data) {
  if (!terminal)
    return;
  if (pid == -1)
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
  GtkWidget *window, *terminal;

  /* Initialise GTK, the window and the terminal */
  gtk_init(&argc, &argv);
  terminal = vte_terminal_new();
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "myterm");

#define CLR_R(x) (((x)&0xff0000) >> 16)
#define CLR_G(x) (((x)&0x00ff00) >> 8)
#define CLR_B(x) (((x)&0x0000ff) >> 0)
#define CLR_16(x) ((double)(x) / 0xff)
#define CLR_GDK(x)                                                             \
  (const GdkRGBA) {                                                            \
    .red = CLR_16(CLR_R(x)), .green = CLR_16(CLR_G(x)),                        \
    .blue = CLR_16(CLR_B(x)), .alpha = 0                                       \
  }
#define CLRA_GDK(x, a)                                                         \
  (const GdkRGBA){.red = CLR_16(CLR_R(x)),                                     \
                  .green = CLR_16(CLR_G(x)),                                   \
                  .blue = CLR_16(CLR_B(x)),                                    \
                  .alpha = a}

  vte_terminal_set_colors(
      VTE_TERMINAL(terminal), &CLR_GDK(0xffffff), &CLRA_GDK(0x000000, 0.85),
      (const GdkRGBA[]){CLR_GDK(0xf2777a),
                        CLR_GDK(0xf99157), CLR_GDK(0xffcc66), CLR_GDK(0x99cc99),
                        CLR_GDK(0x66cccc), CLR_GDK(0x6699cc), CLR_GDK(0xcc99cc),
                        CLR_GDK(0xa3685a), CLR_GDK(0xf2777a),
                        CLR_GDK(0xf99157), CLR_GDK(0xffcc66), CLR_GDK(0x99cc99),
                        CLR_GDK(0x66cccc), CLR_GDK(0x6699cc), CLR_GDK(0xcc99cc),
                        CLR_GDK(0xa3685a)},
      16);

  /* Start a new shell */
  gchar **envp = g_get_environ();
  gchar **command =
      (gchar *[]){g_strdup(g_environ_getenv(envp, "SHELL")), NULL};
  g_strfreev(envp);
  vte_terminal_spawn_async(VTE_TERMINAL(terminal), VTE_PTY_DEFAULT,
                           NULL,        /* working directory  */
                           command,     /* command */
                           NULL,        /* environment */
                           0,           /* spawn flags */
                           NULL, NULL,  /* child setup */
                           NULL,        /* child pid */
                           -1,          /* timeout */
                           NULL,        /* cancellable */
                           child_ready, /* callback */
                           NULL);       /* user_data */

  /* Connect some signals */
  g_signal_connect(window, "delete-event", gtk_main_quit, NULL);
  g_signal_connect(terminal, "child-exited", gtk_main_quit, NULL);
  g_signal_connect(terminal, "key-press-event", G_CALLBACK(on_key_press),
                   GTK_WINDOW(window));
  g_signal_connect(terminal, "char-size-changed",
                   G_CALLBACK(on_char_size_changed), NULL);

  reset_font_size(VTE_TERMINAL(terminal));
  /* Put widgets together and run the main loop */
  gtk_container_add(GTK_CONTAINER(window), terminal);
  gtk_widget_show_all(window);
  gtk_main();
}
