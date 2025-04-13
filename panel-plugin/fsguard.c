// Copyright {{{ 

/*
 * Copyright (c) 2003 Andre Lerche <a.lerche@gmx.net>
 * Copyright (c) 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
 * Copyright (c) 2007, 2008 Mike Massonnet <mmassonnet@xfce.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// }}}

// some includes and defines {{{

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_XFCE_REVISION_H
#include "xfce-revision.h"
#endif

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(__linux__) || defined(__GNU__)
#include <sys/vfs.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__FreeBSD_kernel__)
#include <sys/param.h>
#include <sys/mount.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/libxfce4panel.h>

#define ICON_NORMAL             0
#define ICON_WARNING            1
#define ICON_URGENT             2
#define ICON_INSENSITIVE        3

#define BORDER                  8

#define COLOR_NORMAL            "#00C000"
#define COLOR_WARNING           "#FFE500"
#define COLOR_URGENT            "#FF4F00"

// }}}

// struct {{{

typedef struct
{
    XfcePanelPlugin    *plugin;
    gboolean            seen;
    gint                icon_id;
    gchar              *css_class;
    gint                timeout;
    guint               limit_warning;
    guint               limit_urgent;
    gboolean            show_size;
    gboolean            show_progress_bar;
    gboolean            hide_button;
    gboolean            show_name;
    gchar              *name;
    gchar              *path;

    GtkWidget          *ebox;
    GtkWidget          *box;
    GtkWidget          *btn_panel;
    GtkWidget          *icon_panel;
    GtkWidget          *lab_box;
    GtkWidget          *lab_name;
    GtkWidget          *lab_size;
    GtkWidget          *pb_box;
    GtkWidget          *progress_bar;
    GtkWidget          *cb_hide_button;
} FsGuard;

// }}}

// all functions {{{

static void
fsguard_refresh_button (FsGuard *fsguard)
{
    /* Refresh the checkbox state as seen in the dialog */
    if (fsguard->hide_button == TRUE && (*(fsguard->name) == '\0' || !fsguard->show_name)
        && !fsguard->show_size && !fsguard->show_progress_bar) {
        DBG ("Show the button back");
        if (G_LIKELY (GTK_IS_WIDGET (fsguard->cb_hide_button)))
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fsguard->cb_hide_button), TRUE);
        else {
            gtk_widget_show (fsguard->btn_panel);
            fsguard->hide_button = FALSE;
        }
    }
}

static void
fsguard_refresh_name (FsGuard *fsguard)
{
    if (*(fsguard->name) != '\0' && fsguard->show_name) {
        gtk_label_set_text (GTK_LABEL(fsguard->lab_name), fsguard->name);
        gtk_widget_show (fsguard->lab_name);
    } else {
        gtk_widget_hide (fsguard->lab_name);
        fsguard_refresh_button (fsguard);
    }
}

static void
fsguard_set_icon (FsGuard *fsguard, gint id)
{
    GtkIconTheme       *icon_theme;
    GdkPixbuf          *pixbuf;
    GdkPixbuf          *scaled;
    cairo_surface_t    *surface;
    gint                size;
    gint                scale_factor;

    if (id == fsguard->icon_id)
        return;

    DBG ("icon id: new=%d, cur=%d", id, fsguard->icon_id);
    fsguard->icon_id = id;
    size = xfce_panel_plugin_get_size (fsguard->plugin);
    size /= xfce_panel_plugin_get_nrows (fsguard->plugin);

    size -= 2;

    icon_theme = gtk_icon_theme_get_default ();
    scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (fsguard->plugin));
    if (id == ICON_URGENT) {
        pixbuf = gtk_icon_theme_load_icon_for_scale (icon_theme, "xfce4-fsguard-plugin-urgent", size, scale_factor, 0, NULL);
    } else if (id == ICON_WARNING) {
        pixbuf = gtk_icon_theme_load_icon_for_scale (icon_theme, "xfce4-fsguard-plugin-warning", size, scale_factor, 0, NULL);
    } else {
        pixbuf = gtk_icon_theme_load_icon_for_scale (icon_theme, "xfce4-fsguard-plugin", size, scale_factor, 0, NULL);
    }

    if (G_UNLIKELY (NULL == pixbuf)) {
        pixbuf = gtk_icon_theme_load_icon_for_scale (icon_theme, "gtk-harddisk", size, scale_factor, 0, NULL);
    }

    if (G_UNLIKELY (NULL == pixbuf)) {
        gtk_image_clear (GTK_IMAGE (fsguard->icon_panel));
        return;
    }

    scaled = gdk_pixbuf_scale_simple (pixbuf, size * scale_factor, size * scale_factor, GDK_INTERP_BILINEAR);
    g_object_unref (G_OBJECT (pixbuf));
    pixbuf = scaled;

    surface = gdk_cairo_surface_create_from_pixbuf (pixbuf, scale_factor, NULL);
    gtk_image_set_from_surface (GTK_IMAGE (fsguard->icon_panel), surface);
    gtk_widget_set_sensitive (fsguard->icon_panel, id != ICON_INSENSITIVE);
    cairo_surface_destroy (surface);
    g_object_unref (G_OBJECT (pixbuf));
}

static void
fsguard_refresh_icon (FsGuard *fsguard)
{
    gint icon_id = fsguard->icon_id;
    fsguard->icon_id = -1;
    fsguard_set_icon (fsguard, icon_id);
}

static void
fsguard_refresh_monitor_color (FsGuard *fsguard, gchar *css_class)
{
    GdkRGBA             color;

    switch (fsguard->icon_id) {
      default:
      case ICON_NORMAL:
        gdk_rgba_parse (&color, COLOR_NORMAL);
        break;

      case ICON_WARNING:
        gdk_rgba_parse (&color, COLOR_WARNING);
        break;

      case ICON_URGENT:
        gdk_rgba_parse (&color, COLOR_URGENT);
        break;
    }

    DBG("removing class %s, adding %s", fsguard->css_class, css_class);
    gtk_style_context_remove_class (
        GTK_STYLE_CONTEXT(gtk_widget_get_style_context (GTK_WIDGET (fsguard->progress_bar))),
        fsguard->css_class);
    gtk_style_context_add_class (
        GTK_STYLE_CONTEXT(gtk_widget_get_style_context (GTK_WIDGET (fsguard->progress_bar))),
        css_class);
    g_free(fsguard->css_class);
    fsguard->css_class = g_strdup(css_class);
}

static inline gboolean
__open_mnt (gchar *command, gchar *path)
{
    gboolean res;
    gchar *cmd;
    gchar *path_quoted;
    path_quoted = g_shell_quote (path);
    cmd = g_strdup_printf ("%s %s", command, path_quoted);
    res = xfce_spawn_command_line (NULL, cmd, FALSE, FALSE, TRUE, NULL);
    g_free (path_quoted);
    g_free (cmd);
    return res;
}

static void
fsguard_open_mnt (GtkWidget *widget, FsGuard *fsguard)
{
    GtkWidget *dialog;

    if (fsguard->path == NULL || fsguard->path[0] == '\0')
      return;

#if LIBXFCE4UI_CHECK_VERSION(4, 21, 0)
    if (__open_mnt ("xfce-open", fsguard->path))
#else
    if (__open_mnt ("exo-open", fsguard->path))
#endif
      return;
    if (__open_mnt ("Thunar", fsguard->path))
      return;
    if (__open_mnt ("xdg-open", fsguard->path))
      return;

    dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                     "Free Space Checker");
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                              _("Unable to find an appropriate application to open the mount point"));
}

static void
fsguard_check_fs (FsGuard *fsguard)
{
    float               freespace = 0;
    float               total = 0;
    float               freeblocks = 0;
    float               totalblocks = 0;
    long                blocksize = 0;
    int                 err;
    gchar              *css_class = "normal";
    gchar               msg_size[100], msg_total_size[100], msg[100];
    gint                icon_id = ICON_INSENSITIVE;
    static struct statfs fsd;

    err = statfs (fsguard->path, &fsd);

    if (err != -1) {
        blocksize       = fsd.f_bsize;
        freeblocks      = fsd.f_bavail;
        totalblocks     = fsd.f_blocks;
        freespace       = (freeblocks * blocksize) / 1048576;
        total           = (totalblocks * blocksize) / 1048576;

        if (freespace > (total * fsguard->limit_warning / 100)) {
            icon_id = ICON_NORMAL;
            css_class = "normal";
        } else if (freespace > (total * fsguard->limit_urgent / 100) && freespace <= (total * fsguard->limit_warning / 100)) {
            icon_id = ICON_WARNING;
            css_class = "warning";
        } else {
            icon_id = ICON_URGENT;
            css_class = "urgent";
        }
    }
        g_snprintf (msg, sizeof (msg),
                    _("could not check mountpoint %s, please check your config"),
                    fsguard->path);

    /* msg_total_size, msg_size */
    if (total > 1024) {
        g_snprintf (msg_total_size, sizeof(msg_total_size), _("%.2f GB"), total / 1024);
        g_snprintf (msg_size, sizeof (msg_size), _("%.2f GB"), freespace / 1024);
    } else {
        g_snprintf (msg_total_size, sizeof (msg_total_size), _("%.0f MB"), total);
        g_snprintf (msg_size, sizeof (msg_size), _("%.0f MB"), freespace);
    }
    if (err != -1)
        g_snprintf (msg, sizeof (msg),
                    (*(fsguard->name) != '\0' && strcmp(fsguard->path, fsguard->name)) ?
                    _("%s/%s space left on %s (%s)") : _("%s/%s space left on %s"),
                    msg_size, msg_total_size, fsguard->path, fsguard->name);

    if (fsguard->show_size) {
        gtk_label_set_text (GTK_LABEL(fsguard->lab_size),
                            msg_size);
    }
    if (fsguard->show_progress_bar) {
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(fsguard->progress_bar),
                                       (total > 0 ) ? 1.0 - (freespace / total) : 0.0);
        if (icon_id != fsguard->icon_id)
            fsguard_refresh_monitor_color (fsguard, css_class);
    }

    gtk_widget_set_tooltip_text(fsguard->ebox, msg);
    fsguard_set_icon (fsguard, icon_id);

    if (err != -1 && !fsguard->seen && icon_id == ICON_URGENT) {
        fsguard->seen = TRUE;
        if (*(fsguard->name) != '\0' && strcmp(fsguard->path, fsguard->name)) {
            xfce_dialog_show_warning (NULL, NULL, _("Only %s space left on %s (%s)!"),
                       msg_size, fsguard->path, fsguard->name);
        } else {
            xfce_dialog_show_warning (NULL, NULL, _("Only %s space left on %s!"),
                       msg_size, fsguard->path);
        }
    }
}

static gboolean
fsguard_check_fs_cb (gpointer user_data)
{
    FsGuard *fsguard = user_data;

    fsguard_check_fs (fsguard);
    return TRUE;
}

static void
fsguard_read_config (FsGuard *fsguard)
{
    char               *file;
    XfceRc             *rc;

    /* prepare default values */
    fsguard->seen               = FALSE;
    fsguard->name               = g_strdup ("");
    fsguard->show_name          = FALSE;
    fsguard->path               = g_strdup ("/");
    fsguard->css_class          = g_strdup ("normal");
    fsguard->show_size          = TRUE;
    fsguard->show_progress_bar  = TRUE;
    fsguard->hide_button        = FALSE;
    fsguard->limit_warning      = 8;
    fsguard->limit_urgent       = 2;

    file = xfce_panel_plugin_lookup_rc_file(fsguard->plugin);
    if (!file)
        return;
    DBG ("Lookup rc file `%s'", file);
    rc = xfce_rc_simple_open (file, TRUE);
    g_free (file);
    g_return_if_fail (rc);

    g_free (fsguard->name);
    fsguard->name               = g_strdup (xfce_rc_read_entry (rc, "label", ""));
    fsguard->show_name          = xfce_rc_read_bool_entry (rc, "label_visible", FALSE);
    g_free (fsguard->path);
    fsguard->path               = g_strdup (xfce_rc_read_entry (rc, "mnt", "/"));
    fsguard->show_size          = xfce_rc_read_bool_entry (rc, "lab_size_visible", TRUE);
    fsguard->show_progress_bar  = xfce_rc_read_bool_entry (rc, "progress_bar_visible", TRUE);
    fsguard->hide_button        = xfce_rc_read_bool_entry (rc, "hide_button", FALSE);
    fsguard->limit_warning      = xfce_rc_read_int_entry (rc, "yellow", 8);
    fsguard->limit_urgent       = xfce_rc_read_int_entry (rc, "red", 2);
    /* Prevent MB values from earlier configuration files (2009-08-14) */
    if (fsguard->limit_warning > 100)
      fsguard->limit_warning = 8;
    if (fsguard->limit_urgent > 100)
      fsguard->limit_urgent = 2;

    xfce_rc_close (rc);
}

static void
fsguard_write_config (XfcePanelPlugin *plugin, FsGuard *fsguard)
{
    char               *file;
    XfceRc             *rc;

    file = xfce_panel_plugin_save_location (plugin, TRUE);
    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);
    g_return_if_fail (rc);

    xfce_rc_write_int_entry (rc, "yellow", fsguard->limit_warning);
    xfce_rc_write_int_entry (rc, "red", fsguard->limit_urgent);
    xfce_rc_write_bool_entry (rc, "lab_size_visible", fsguard->show_size);
    xfce_rc_write_bool_entry (rc, "progress_bar_visible", fsguard->show_progress_bar);
    xfce_rc_write_bool_entry (rc, "hide_button", fsguard->hide_button);
    xfce_rc_write_entry (rc, "label", fsguard->name);
    xfce_rc_write_bool_entry (rc, "label_visible", fsguard->show_name);
    xfce_rc_write_entry (rc, "mnt", fsguard->path);

    xfce_rc_close (rc);
}    

static FsGuard *
fsguard_new (XfcePanelPlugin *plugin)
{
    GtkOrientation orientation = xfce_panel_plugin_get_orientation (plugin);
    GtkCssProvider *css_provider;
    FsGuard *fsguard = g_new0(FsGuard, 1);

    fsguard->plugin = plugin;

    fsguard_read_config (fsguard);

    fsguard->ebox = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(fsguard->ebox), FALSE);

    fsguard->box = gtk_box_new (orientation, 2);

    fsguard->lab_name = gtk_label_new (NULL);
    fsguard->lab_size = gtk_label_new (NULL);
    fsguard->lab_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

    gtk_widget_set_halign(fsguard->lab_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(fsguard->lab_box, GTK_ALIGN_CENTER);

    fsguard->btn_panel = xfce_panel_create_button ();
    fsguard->icon_panel = gtk_image_new ();

    fsguard->progress_bar = gtk_progress_bar_new ();
    css_provider = gtk_css_provider_new ();
        gtk_css_provider_load_from_data (css_provider, "\
            progressbar.horizontal trough { min-height: 4px; }\
            progressbar.horizontal progress { min-height: 4px; }\
            progressbar.vertical trough { min-width: 4px; }\
            progressbar.vertical progress { min-width: 4px; }\
            .normal progress { background-color: " COLOR_NORMAL " ; background-image: none; }\
            .warning progress { background-color: " COLOR_WARNING " ; background-image: none; }\
            .urgent progress { background-color: " COLOR_URGENT " ; background-image: none; }",
             -1, NULL);
    gtk_style_context_add_provider (
        GTK_STYLE_CONTEXT (gtk_widget_get_style_context (GTK_WIDGET (fsguard->progress_bar))),
        GTK_STYLE_PROVIDER (css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref (css_provider);
    gtk_style_context_add_class (
        GTK_STYLE_CONTEXT(gtk_widget_get_style_context (GTK_WIDGET (fsguard->progress_bar))),
        fsguard->css_class);
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(fsguard->progress_bar), 0.0);
    gtk_progress_bar_set_inverted (GTK_PROGRESS_BAR(fsguard->progress_bar), (orientation == GTK_ORIENTATION_HORIZONTAL));
    gtk_orientable_set_orientation (GTK_ORIENTABLE(fsguard->progress_bar), !orientation);
    fsguard->pb_box = gtk_box_new (orientation, 0);

    g_signal_connect (G_OBJECT(fsguard->btn_panel),
                      "clicked",
                      G_CALLBACK(fsguard_open_mnt),
                      fsguard);

    gtk_container_add (GTK_CONTAINER(fsguard->ebox), fsguard->box);
    gtk_container_add (GTK_CONTAINER(fsguard->box), fsguard->btn_panel);
    gtk_container_add (GTK_CONTAINER(fsguard->btn_panel), fsguard->icon_panel);
    gtk_container_add (GTK_CONTAINER(fsguard->lab_box), fsguard->lab_name);
    gtk_container_add (GTK_CONTAINER(fsguard->lab_box), fsguard->lab_size);
    gtk_container_add (GTK_CONTAINER(fsguard->box), fsguard->lab_box);
    gtk_container_add (GTK_CONTAINER(fsguard->box), fsguard->pb_box);
    gtk_container_add (GTK_CONTAINER(fsguard->pb_box), fsguard->progress_bar);

    xfce_panel_plugin_add_action_widget (plugin, fsguard->ebox);
    xfce_panel_plugin_add_action_widget (plugin, fsguard->btn_panel);

    gtk_widget_set_size_request(fsguard->ebox, -1, -1);
    gtk_widget_show_all (fsguard->ebox);
    fsguard_refresh_name (fsguard);
    fsguard_refresh_button (fsguard);
    if (fsguard->show_size != TRUE)
        gtk_widget_hide (fsguard->lab_size);
    if (fsguard->show_progress_bar != TRUE)
        gtk_widget_hide (fsguard->pb_box);
    if (fsguard->hide_button != FALSE)
        gtk_widget_hide (fsguard->btn_panel);

    return fsguard;
}

static void
fsguard_free (XfcePanelPlugin *plugin, FsGuard *fsguard)
{
    if (fsguard->timeout != 0) {
        g_source_remove (fsguard->timeout);
    }

    g_free (fsguard->name);
    g_free (fsguard->path);

    g_free(fsguard);
}

static gboolean
fsguard_set_size (XfcePanelPlugin *plugin, int size, FsGuard *fsguard)
{
    int border_width = (size > 26 ? 2 : 1);
    GtkOrientation orientation = xfce_panel_plugin_get_orientation (plugin);

    size /= xfce_panel_plugin_get_nrows (plugin);
    DBG ("Set size to `%d'", size);

    gtk_container_set_border_width (GTK_CONTAINER (fsguard->pb_box), border_width);

    if (orientation == GTK_ORIENTATION_HORIZONTAL) {
        gtk_widget_set_size_request (GTK_WIDGET(fsguard->progress_bar), 8, -1);
        gtk_widget_set_size_request (GTK_WIDGET(plugin), -1, size);
    } else {
        gtk_widget_set_size_request (GTK_WIDGET(fsguard->progress_bar), -1, 8);
        gtk_widget_set_size_request (GTK_WIDGET(plugin), size, -1);
    }
    gtk_widget_set_size_request (fsguard->btn_panel, size, size);
    size -= 2 * border_width;
    gtk_widget_set_size_request (fsguard->icon_panel, size, size);

    fsguard_refresh_icon (fsguard);

    return TRUE;
}

static void
fsguard_set_mode (XfcePanelPlugin *plugin, XfcePanelPluginMode mode, FsGuard *fsguard)
{
    GtkOrientation orientation, panel_orientation;

    orientation =
      (mode != XFCE_PANEL_PLUGIN_MODE_VERTICAL) ?
      GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;
    panel_orientation =
      (mode == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL) ?
      GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;

    DBG ("Set mode to `%s'", mode == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL ?
         "Horizontal" : (mode == XFCE_PANEL_PLUGIN_MODE_VERTICAL ? "Vertical" : "Deskbar"));

    gtk_orientable_set_orientation (GTK_ORIENTABLE (fsguard->box), panel_orientation);
    gtk_orientable_set_orientation (GTK_ORIENTABLE (fsguard->pb_box), panel_orientation);
    gtk_orientable_set_orientation (GTK_ORIENTABLE (fsguard->progress_bar), !panel_orientation);
    gtk_progress_bar_set_inverted (GTK_PROGRESS_BAR(fsguard->progress_bar), (panel_orientation == GTK_ORIENTATION_HORIZONTAL));
    gtk_label_set_angle (GTK_LABEL(fsguard->lab_name),
                         orientation == GTK_ORIENTATION_VERTICAL ? -90 : 0);
    gtk_label_set_angle (GTK_LABEL(fsguard->lab_size),
                         orientation == GTK_ORIENTATION_VERTICAL ? -90 : 0);
    gtk_orientable_set_orientation (GTK_ORIENTABLE (fsguard->lab_box),
                                orientation == GTK_ORIENTATION_VERTICAL ?
                                GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);
    gtk_box_reorder_child (GTK_BOX (fsguard->lab_box),
                           orientation == GTK_ORIENTATION_VERTICAL ? fsguard->lab_size : fsguard->lab_name, 0);
    fsguard_set_size (plugin, xfce_panel_plugin_get_size (plugin), fsguard);
}

static void
fsguard_entry1_changed (GtkWidget *widget, FsGuard *fsguard)
{
    g_free (fsguard->path);
    fsguard->path = g_strdup (gtk_entry_get_text (GTK_ENTRY(widget)));
    fsguard->seen = FALSE;
    fsguard_check_fs (fsguard);
}

static void
fsguard_spin1_changed (GtkWidget *widget, FsGuard *fsguard)
{
    fsguard->limit_warning = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(widget));
    fsguard->seen = FALSE;
    fsguard_check_fs (fsguard);
}

static void
fsguard_spin2_changed (GtkWidget *widget, FsGuard *fsguard)
{
    fsguard->limit_urgent = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(widget));
    fsguard_check_fs (fsguard);
}

static void
fsguard_check1_changed (GtkWidget *widget, FsGuard *fsguard)
{
    fsguard->show_name = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(widget));
    fsguard_refresh_name (fsguard);
}

static void
fsguard_entry3_changed (GtkWidget *widget, FsGuard *fsguard)
{
    g_free (fsguard->name);
    fsguard->name = g_strdup (gtk_entry_get_text (GTK_ENTRY(widget)));
    fsguard_refresh_name (fsguard);
}

static void
fsguard_check2_changed (GtkWidget *widget, FsGuard *fsguard)
{
    fsguard->show_size = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(widget));
    if (fsguard->show_size)
        gtk_widget_show (fsguard->lab_size);
    else {
        gtk_widget_hide (fsguard->lab_size);
        fsguard_refresh_button (fsguard);
    }
}

static void
fsguard_check3_changed (GtkWidget *widget, FsGuard *fsguard)
{
    fsguard->show_progress_bar = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(widget));
    if (fsguard->show_progress_bar)
        gtk_widget_show (fsguard->pb_box);
    else {
        gtk_widget_hide (fsguard->pb_box);
        fsguard_refresh_button (fsguard);
    }
}

static void
fsguard_check4_changed (GtkWidget *widget, FsGuard *fsguard)
{
    fsguard->hide_button = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(widget));

    if (fsguard->hide_button == FALSE)
        gtk_widget_show (fsguard->btn_panel);
    else {
        gtk_widget_hide (fsguard->btn_panel);
        fsguard_refresh_button (fsguard);
    }
}

static void
fsguard_create_options (XfcePanelPlugin *plugin, FsGuard *fsguard)
{
    GtkWidget *dialog;
    GtkWidget *area;
    GtkWidget *table1;
    GtkWidget *frame1;
    GtkWidget *alignment;
    GtkWidget *label1;
    GtkWidget *entry1;
    GtkWidget *label3;
    GtkWidget *spin1;
    GtkWidget *label4;
    GtkWidget *spin2;
    GtkWidget *table2;
    GtkWidget *frame2;
    GtkWidget *check1;
    GtkWidget *entry3;
    GtkWidget *check2;
    GtkWidget *check3;
    GtkSizeGroup *size_group;

    xfce_panel_plugin_block_menu (plugin);

    /* Dialog */
    dialog =
      xfce_titled_dialog_new_with_mixed_buttons (_("Free Space Checker"),
        GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        "window-close-symbolic", _("_Close"), GTK_RESPONSE_OK,
        NULL);
    gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-fsguard-plugin-warning");
    gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

    size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_set_spacing (GTK_BOX (area), 18);
    gtk_container_set_border_width (GTK_CONTAINER (area), 12);

    /* Configuration frame */
    table1 = gtk_grid_new ();
    frame1 = xfce_gtk_frame_box_new (_("Configuration"), &alignment);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 6, 0, 12, 0);
    gtk_container_add (GTK_CONTAINER (alignment), table1);
    gtk_grid_set_row_spacing (GTK_GRID (table1), 6);
    gtk_grid_set_column_spacing (GTK_GRID (table1), 12);
    gtk_box_pack_start (GTK_BOX (area), frame1, TRUE, TRUE, 0);

    label1 = gtk_label_new (_("Mount point"));
    gtk_widget_set_valign(label1, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label1), 0.0f);
    entry1 = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (entry1), fsguard->path);

    gtk_size_group_add_widget (size_group, label1);

    label3 = gtk_label_new (_("Warning limit (%)"));
    gtk_widget_set_valign(label3, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label3), 0.0f);
    spin1 = gtk_spin_button_new_with_range (0, 100, 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin1), fsguard->limit_warning);
    gtk_widget_set_halign (GTK_WIDGET (spin1), GTK_ALIGN_START);

    gtk_size_group_add_widget (size_group, label3);

    label4 = gtk_label_new (_("Urgent limit (%)"));
    gtk_widget_set_valign(label4, GTK_ALIGN_CENTER);
    gtk_label_set_xalign (GTK_LABEL (label4), 0.0f);
    spin2 = gtk_spin_button_new_with_range (0, 100, 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin2), fsguard->limit_urgent);
    gtk_widget_set_halign (GTK_WIDGET (spin2), GTK_ALIGN_START);

    gtk_size_group_add_widget (size_group, label4);

    gtk_grid_attach (GTK_GRID (table1), label1,
                               0, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (table1), entry1,
                               1, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (table1), label3,
                               0, 1, 1, 1);
    gtk_grid_attach (GTK_GRID (table1), spin1,
                               1, 1, 1, 1);
    gtk_grid_attach (GTK_GRID (table1), label4,
                               0, 2, 1, 1);
    gtk_grid_attach (GTK_GRID (table1), spin2,
                               1, 2, 1, 1);

    /* Display frame */
    table2 = gtk_grid_new ();
    frame2 = xfce_gtk_frame_box_new (_("User Interface"), &alignment);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 6, 0, 12, 0);
    gtk_container_add (GTK_CONTAINER (alignment), table2);

    gtk_grid_set_row_spacing (GTK_GRID (table2), 6);
    gtk_grid_set_column_spacing (GTK_GRID (table2), 12);
    gtk_box_pack_start (GTK_BOX (area), frame2, TRUE, TRUE, 0);

    check1 = gtk_check_button_new_with_label (_("Name"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check1),
                                  fsguard->show_name);

    gtk_size_group_add_widget (size_group, check1);

    entry3 = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (entry3), 16);
    gtk_entry_set_text (GTK_ENTRY (entry3), fsguard->name);

    check2 = gtk_check_button_new_with_label (_("Display size"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check2),
                                  fsguard->show_size);

    check3 = gtk_check_button_new_with_label (_("Display meter"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check3),
                                  fsguard->show_progress_bar);

    fsguard->cb_hide_button = gtk_check_button_new_with_label (_("Display button"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fsguard->cb_hide_button),
                                  !fsguard->hide_button);

    gtk_grid_attach (GTK_GRID (table2), check1,
                               0, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (table2), entry3,
                               1, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (table2), check2,
                               0, 1, 2, 1);
    gtk_grid_attach (GTK_GRID (table2), check3,
                               0, 2, 2, 1);
    gtk_grid_attach (GTK_GRID (table2), fsguard->cb_hide_button,
                               0, 3, 2, 1);

    g_signal_connect (entry1,
                      "changed",
                      G_CALLBACK (fsguard_entry1_changed),
                      fsguard);
    g_signal_connect (spin1,
                      "value-changed",
                      G_CALLBACK (fsguard_spin1_changed),
                      fsguard);
    g_signal_connect (spin2,
                      "value-changed",
                      G_CALLBACK (fsguard_spin2_changed),
                      fsguard);
    g_signal_connect (check1,
                      "toggled",
                      G_CALLBACK (fsguard_check1_changed),
                      fsguard);
    g_signal_connect (entry3,
                      "changed",
                      G_CALLBACK (fsguard_entry3_changed),
                      fsguard);
    g_signal_connect (check2,
                      "toggled",
                      G_CALLBACK (fsguard_check2_changed),
                      fsguard);
    g_signal_connect (check3,
                      "toggled",
                      G_CALLBACK (fsguard_check3_changed),
                      fsguard);
    g_signal_connect (fsguard->cb_hide_button,
                      "toggled",
                      G_CALLBACK (fsguard_check4_changed),
                      fsguard);

    gtk_widget_show_all (GTK_WIDGET (dialog));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    xfce_panel_plugin_unblock_menu (fsguard->plugin);
    fsguard_write_config (fsguard->plugin, fsguard);
}

static void
fsguard_show_about (XfcePanelPlugin *plugin, FsGuard *fsguard)
{
    const gchar *auth[] = {
        "Andre Lerche <a.lerche@gmx.net>",
        "Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>",
        "Mike Massonnet <mmassonnet@xfce.org>",
        NULL
    };

    gtk_show_about_dialog (NULL,
        "logo-icon-name", "xfce4-fsguard-plugin",
        "license", xfce_get_license_text (XFCE_LICENSE_TEXT_BSD),
        "version", VERSION_FULL,
        "program-name", PACKAGE_NAME,
        "comments", _("Monitor free disk space"),
        "website", PACKAGE_URL,
        "copyright", "Copyright \302\251 2003-" COPYRIGHT_YEAR " The Xfce development team",
        "authors", auth, NULL);
}

// }}}

// initialization {{{
static void
fsguard_construct (XfcePanelPlugin *plugin)
{
    FsGuard *fsguard;

    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    fsguard = fsguard_new (plugin);
 
    fsguard_check_fs (fsguard);
    fsguard->timeout = g_timeout_add (8192, fsguard_check_fs_cb, fsguard);

    gtk_container_add (GTK_CONTAINER (plugin), fsguard->ebox);
    fsguard_set_size(fsguard->plugin, xfce_panel_plugin_get_size(fsguard->plugin), fsguard);

    g_signal_connect (plugin,
                      "free-data",
                      G_CALLBACK (fsguard_free),
                      fsguard);
    g_signal_connect (plugin,
                      "save",
                      G_CALLBACK (fsguard_write_config),
                      fsguard);
    g_signal_connect (plugin,
                      "size-changed",
                      G_CALLBACK (fsguard_set_size),
                      fsguard);
    g_signal_connect (plugin,
                      "mode-changed",
                      G_CALLBACK (fsguard_set_mode),
                      fsguard);
    xfce_panel_plugin_set_small (plugin, TRUE);
    g_signal_connect (plugin,
                      "configure-plugin",
                      G_CALLBACK (fsguard_create_options),
                      fsguard);
    g_signal_connect (plugin,
                      "about",
                      G_CALLBACK (fsguard_show_about),
                      fsguard);

    xfce_panel_plugin_menu_show_configure (plugin);
    xfce_panel_plugin_menu_show_about (plugin);
}

XFCE_PANEL_PLUGIN_REGISTER (fsguard_construct);

// }}}
