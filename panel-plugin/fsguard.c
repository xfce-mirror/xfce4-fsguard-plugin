// Copyright {{{ 

/*
 * Copyright (c) 2003 Andre Lerche <a.lerche@gmx.net>
 * Copyright (c) 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
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

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(__linux__)
#include <sys/vfs.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__FreeBSD_kernel__)
#include <sys/param.h>
#include <sys/mount.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-convenience.h>
#include <libxfce4panel/xfce-hvbox.h>

#define ICON_NORMAL             0
#define ICON_WARNING            1
#define ICON_URGENT             2
#define ICON_INSENSITIVE        3

#define BORDER 8

// }}}

// struct {{{

typedef struct
{
    XfcePanelPlugin    *plugin;
    gboolean            seen;
    gint                icon_id;
    gint                timeout;
    gint                limit_warning;
    gint                limit_urgent;
    gboolean            show_size;
    gboolean            show_progress_bar;
    gchar              *name;
    gchar              *path;
    gchar              *filemanager;

    GtkWidget          *ebox;
    GtkWidget          *box;
    GtkWidget          *btn_panel;
    GtkWidget          *icon_panel;
    GtkWidget          *lab_name;
    GtkWidget          *lab_size;
    GtkWidget          *pb_box;
    GtkWidget          *progress_bar;
} FsGuard;

static GtkTooltips *tooltips = NULL;

// }}}

// all functions {{{

static void
fsguard_refresh_name (FsGuard *fsguard)
{
    if (fsguard->name != NULL && (strlen (fsguard->name) > 0)) {
        gtk_label_set_text (GTK_LABEL(fsguard->lab_name), fsguard->name);
        gtk_widget_show (fsguard->lab_name);
    } else {
        gtk_widget_hide (fsguard->lab_name);
    }
}

static void
fsguard_set_icon (FsGuard *fsguard, gint id)
{
    GdkPixbuf          *pixbuf;
    gint                size;

    if (id == fsguard->icon_id)
        return;

    fsguard->icon_id = id;
    size = xfce_panel_plugin_get_size (fsguard->plugin);
    size = size - 2 - (2 * MAX (fsguard->btn_panel->style->xthickness,
                                fsguard->btn_panel->style->ythickness));

    switch (id) {
      default:
      case ICON_NORMAL:
        pixbuf = xfce_themed_icon_load ("xfce4-fsguard-plugin", size);
        break;
      case ICON_WARNING:
        pixbuf = xfce_themed_icon_load ("xfce4-fsguard-plugin-warning", size);
        break;
      case ICON_URGENT:
        pixbuf = xfce_themed_icon_load ("xfce4-fsguard-plugin-urgent", size);
        break;
    }

    gtk_widget_set_sensitive (fsguard->icon_panel, id != ICON_INSENSITIVE);
    gtk_image_set_from_pixbuf (GTK_IMAGE (fsguard->icon_panel), pixbuf);
    g_object_unref (G_OBJECT (pixbuf));
}

static inline void
fsguard_refresh_icon (FsGuard *fsguard)
{
    gint icon_id = fsguard->icon_id;
    fsguard->icon_id = -1;
    fsguard_set_icon (fsguard, icon_id);
}

static void
fsguard_open_mnt (GtkWidget *widget, FsGuard *fsguard)
{
    GString *cmd;
    if (strlen(fsguard->filemanager) == 0) {
        return;
    }
    cmd = g_string_new (fsguard->filemanager);
    if (fsguard->path != NULL && (strcmp(fsguard->path, ""))) {
        g_string_append (cmd, " ");
        g_string_append (cmd, fsguard->path);
    }
    xfce_exec (cmd->str, FALSE, FALSE, NULL);
    g_string_free (cmd, TRUE);
}

static gboolean
fsguard_check_fs (FsGuard *fsguard)
{
    float               size = 0;
    float               total = 0;
    float               freeblocks = 0;
    float               totalblocks = 0;
    long                blocksize;
    int                 err;
    gchar               msg_size[100], msg_total_size[100], msg[100];
    gint                icon_id = ICON_INSENSITIVE;
    static struct statfs fsd;

    err = statfs (fsguard->path, &fsd);
    
    if (err != -1) {
        blocksize       = fsd.f_bsize;
        freeblocks      = fsd.f_bavail;
        totalblocks     = fsd.f_blocks;
        size            = (freeblocks * blocksize) / 1048576;
        total           = (totalblocks * blocksize) / 1048576;

        if (total > 1024) {
            g_snprintf (msg_total_size, sizeof (msg_total_size), _("%.2f GB"), total/1024);
        } else {
            g_snprintf (msg_total_size, sizeof (msg_total_size), _("%.2f MB"), total);
        }
        if (size > 1024) {
            g_snprintf (msg_size, sizeof (msg_size), _("%.2f GB"), size/1024);
        } else {
            g_snprintf (msg_size, sizeof (msg_size), _("%.2f MB"), size);
        }
        gtk_label_set_text (GTK_LABEL(fsguard->lab_size), msg_size);
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(fsguard->progress_bar), size / total);

        if (size <= fsguard->limit_urgent) {
            icon_id = ICON_URGENT;
	    if (!fsguard->seen) {
                if (fsguard->name != NULL && (strcmp(fsguard->name,"")) && (strcmp(fsguard->path, fsguard->name))) {
                    xfce_warn (_("Only %s space left on %s (%s)!"), msg_size, fsguard->path, fsguard->name);
                } else {
                    xfce_warn (_("Only %s space left on %s!"), msg_size, fsguard->path);
		}
		fsguard->seen = TRUE;
	    }
        } else if (size >= fsguard->limit_urgent && size <= fsguard->limit_warning) {
            icon_id = ICON_WARNING;
        } else {
            icon_id = ICON_NORMAL;
        }

        if (fsguard->name != NULL && (strcmp(fsguard->name,"")) && (strcmp(fsguard->path, fsguard->name))) {
            g_snprintf (msg, sizeof (msg), _("%s/%s space left on %s (%s)"), msg_size, msg_total_size, fsguard->path, fsguard->name);
        } else if (fsguard->path != NULL && (strcmp(fsguard->path, ""))) {
            g_snprintf (msg, sizeof (msg), _("%s/%s space left on %s"), msg_size, msg_total_size, fsguard->path);
        } 
    } else {
        gtk_label_set_text (GTK_LABEL(fsguard->lab_size), "0.0 MB");
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(fsguard->progress_bar), 0.0);
        g_snprintf (msg, sizeof (msg), _("could not check mountpoint %s, please check your config"), fsguard->path);
    }
    
    gtk_tooltips_set_tip (tooltips, fsguard->ebox, msg, NULL);
    fsguard_set_icon (fsguard, icon_id);

    return TRUE;
}

static void
fsguard_read_config (FsGuard *fsguard)
{
    char               *file;
    XfceRc             *rc;

    file = xfce_panel_plugin_save_location (fsguard->plugin, TRUE);
    DBG ("Lookup rc file `%s'", file);
    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    fsguard->seen               = FALSE;
    fsguard->name               = g_strdup (xfce_rc_read_entry (rc, "label", ""));
    fsguard->path               = g_strdup (xfce_rc_read_entry (rc, "mnt", "/"));
    fsguard->filemanager        = g_strdup (xfce_rc_read_entry (rc, "filemanager", "Thunar"));
    fsguard->show_size          = xfce_rc_read_bool_entry (rc, "lab_size_visible", TRUE);
    fsguard->show_progress_bar  = xfce_rc_read_bool_entry (rc, "progress_bar_visible", TRUE);
    fsguard->limit_warning      = xfce_rc_read_int_entry (rc, "yellow", 1500);
    fsguard->limit_urgent       = xfce_rc_read_int_entry (rc, "red", 300);

    xfce_rc_close (rc);
}

static void
fsguard_write_config (XfcePanelPlugin *plugin, FsGuard *fsguard)
{
    char               *file;
    XfceRc             *rc;

    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
        return;
    
    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc)
        return;
    
    xfce_rc_write_int_entry (rc, "yellow", fsguard->limit_warning);
    xfce_rc_write_int_entry (rc, "red", fsguard->limit_urgent);
    xfce_rc_write_bool_entry (rc, "lab_size_visible", fsguard->show_size);
    xfce_rc_write_bool_entry (rc, "progress_bar_visible", fsguard->show_progress_bar);
    xfce_rc_write_entry (rc, "label", fsguard->name);
    xfce_rc_write_entry (rc, "mnt", fsguard->path);
    xfce_rc_write_entry (rc, "filemanager", fsguard->filemanager);

    xfce_rc_close (rc);
}    

static FsGuard *
fsguard_new (XfcePanelPlugin *plugin)
{
    FsGuard *fsguard = g_new0(FsGuard, 1);
    fsguard->plugin = plugin;

    fsguard_read_config (fsguard);

    tooltips = gtk_tooltips_new ();

    fsguard->ebox = gtk_event_box_new();

    GtkOrientation orientation = xfce_panel_plugin_get_orientation (plugin);
    fsguard->box =
      xfce_hvbox_new (orientation == GTK_ORIENTATION_HORIZONTAL ?
                      GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL,
                      FALSE, 2);

    fsguard->lab_name = gtk_label_new (fsguard->name);

    fsguard->btn_panel = xfce_create_panel_button ();
    fsguard->icon_panel = gtk_image_new ();

    fsguard->progress_bar = gtk_progress_bar_new ();
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(fsguard->progress_bar), 0.0);
    gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR(fsguard->progress_bar),
                                      orientation == GTK_ORIENTATION_HORIZONTAL ?
                                      GTK_PROGRESS_BOTTOM_TO_TOP : GTK_PROGRESS_LEFT_TO_RIGHT);

    fsguard->pb_box =
      xfce_hvbox_new (orientation == GTK_ORIENTATION_HORIZONTAL ?
                      GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL,
                      FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (fsguard->pb_box), 2);

    fsguard->lab_size = gtk_label_new (NULL);

    g_signal_connect (G_OBJECT(fsguard->btn_panel),
                      "clicked",
                      G_CALLBACK(fsguard_open_mnt),
                      fsguard);

    gtk_container_add (GTK_CONTAINER(fsguard->ebox), fsguard->box);
    gtk_container_add (GTK_CONTAINER(fsguard->box), fsguard->lab_name);
    gtk_container_add (GTK_CONTAINER(fsguard->box), fsguard->btn_panel);
    gtk_container_add (GTK_CONTAINER(fsguard->btn_panel), fsguard->icon_panel);
    gtk_container_add (GTK_CONTAINER(fsguard->box), fsguard->lab_size);
    gtk_container_add (GTK_CONTAINER(fsguard->box), fsguard->pb_box);
    gtk_container_add (GTK_CONTAINER(fsguard->pb_box), fsguard->progress_bar);

    xfce_panel_plugin_add_action_widget (plugin, fsguard->ebox);
    xfce_panel_plugin_add_action_widget (plugin, fsguard->btn_panel);

    gtk_widget_set_size_request(fsguard->ebox, -1, -1);
    gtk_widget_show_all (fsguard->ebox);
    fsguard_refresh_name (fsguard);
    if (fsguard->show_size != TRUE)
        gtk_widget_hide (fsguard->lab_size);
    if (fsguard->show_progress_bar != TRUE)
        gtk_widget_hide (fsguard->pb_box);

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
    g_free (fsguard->filemanager);

    g_free(fsguard);
}

static void
fsguard_set_orientation (XfcePanelPlugin *plugin, GtkOrientation orientation, FsGuard *fsguard)
{
    DBG ("Set orientation to `%s'", orientation == GTK_ORIENTATION_HORIZONTAL ?
                                    "Horizontal" : "Vertical");

    xfce_hvbox_set_orientation (XFCE_HVBOX (fsguard->box), orientation);
    xfce_hvbox_set_orientation (XFCE_HVBOX (fsguard->pb_box), orientation);
    gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR(fsguard->progress_bar),
                                      orientation == GTK_ORIENTATION_HORIZONTAL ?
                                      GTK_PROGRESS_BOTTOM_TO_TOP : GTK_PROGRESS_LEFT_TO_RIGHT);
}

static gboolean
fsguard_set_size (XfcePanelPlugin *plugin, int size, FsGuard *fsguard)
{
    DBG ("Set size to `%d'", size);

    gtk_widget_set_size_request (fsguard->btn_panel, size, size);

    GtkOrientation orientation = xfce_panel_plugin_get_orientation (plugin);
    if (orientation == GTK_ORIENTATION_HORIZONTAL) {
        gtk_widget_set_size_request (GTK_WIDGET(fsguard->progress_bar), BORDER, -1);
    } else {
        gtk_widget_set_size_request (GTK_WIDGET(fsguard->progress_bar), -1, BORDER);
    }

    gtk_widget_set_size_request (GTK_WIDGET(plugin), -1, -1);
    fsguard_refresh_icon (fsguard);

    return TRUE;
}

static void
fsguard_ent1_changed (GtkWidget *widget, FsGuard *fsguard)
{
    g_free (fsguard->name);
    fsguard->name = g_strdup (gtk_entry_get_text (GTK_ENTRY(widget)));
    fsguard_refresh_name (fsguard);
}

static void
fsguard_ent2_changed (GtkWidget *widget, FsGuard *fsguard)
{
    g_free (fsguard->path);
    fsguard->path = g_strdup (gtk_entry_get_text (GTK_ENTRY(widget)));
    fsguard->seen = FALSE;
    fsguard_check_fs (fsguard);
}

static void
fsguard_ent3_changed (GtkWidget *widget, FsGuard *fsguard)
{
    g_free (fsguard->filemanager);
    fsguard->filemanager = g_strdup (gtk_entry_get_text (GTK_ENTRY(widget)));
}

static void
fsguard_spin1_changed (GtkWidget *widget, FsGuard *fsguard)
{
    fsguard->limit_warning = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(widget));
    fsguard->seen = FALSE;
}

static void
fsguard_spin2_changed (GtkWidget *widget, FsGuard *fsguard)
{
    fsguard->limit_urgent = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(widget));
}

static void
fsguard_check1_changed (GtkWidget *widget, FsGuard *fsguard)
{
    fsguard->show_size = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(widget));
    if (fsguard->show_size)
        gtk_widget_show (fsguard->lab_size);
    else
        gtk_widget_hide (fsguard->lab_size);
}

static void
fsguard_check2_changed (GtkWidget *widget, FsGuard *fsguard)
{
    fsguard->show_progress_bar = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(widget));
    if (fsguard->show_progress_bar)
        gtk_widget_show (fsguard->pb_box);
    else
        gtk_widget_hide (fsguard->pb_box);
}

static void
fsguard_dialog_response (GtkWidget *dlg, int response, FsGuard *fsguard)
{
    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (fsguard->plugin);
    fsguard_write_config (fsguard->plugin, fsguard);
}

static void
fsguard_create_options (XfcePanelPlugin *plugin, FsGuard *fsguard)
{
    GtkWidget *dlg;
    GtkWidget *hbox, *vbox1, *vbox2, *spin1, *spin2;
    GtkWidget *lab1, *lab2, *lab3, *lab4, *lab5, *lab6, *lab7;
    GtkWidget *ent1, *ent2, *ent3;
    GtkWidget *check1, *check2;
    gchar *text[] = {
	     N_("Name"),
	     N_("Mount point"),
             N_("Warning limit (MB)"),
	     N_("Urgent limit (MB)"),
	     N_("File manager"),
	     N_("Display size"),
	     N_("Display progress bar"),
    };
    xfce_panel_plugin_block_menu (plugin);
    

    dlg = xfce_titled_dialog_new_with_buttons (_("Free Space Checker"), 
                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);
    
    g_signal_connect (dlg, "response", G_CALLBACK (fsguard_dialog_response),
                      fsguard);

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);
    
    gtk_window_set_icon_name (GTK_WINDOW (dlg), "xfce4-settings");
    
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), BORDER - 2);
    
    vbox1 = gtk_vbox_new (FALSE, BORDER);
    vbox2 = gtk_vbox_new (FALSE, BORDER);

    gtk_box_pack_start (GTK_BOX(hbox), vbox1, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(hbox), vbox2, TRUE, FALSE, 1);

    lab1 = gtk_label_new (_(text[0]));
    lab2 = gtk_label_new (_(text[1]));
    lab3 = gtk_label_new (_(text[2]));
    lab4 = gtk_label_new (_(text[3]));
    lab5 = gtk_label_new (_(text[4]));
    lab6 = gtk_label_new (NULL);
    lab7 = gtk_label_new (NULL);

    ent1 = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY(ent1), 16);
    gtk_entry_set_text (GTK_ENTRY(ent1), fsguard->name);

    ent2 = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY(ent2), 32);
    gtk_entry_set_text (GTK_ENTRY(ent2), fsguard->path);

    ent3 = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY(ent3), 16);
    gtk_entry_set_text (GTK_ENTRY(ent3), fsguard->filemanager);

    check1 = gtk_check_button_new_with_label (_(text[5]));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check1), fsguard->show_size);
 
    check2 = gtk_check_button_new_with_label (_(text[6]));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check2), fsguard->show_progress_bar);

    spin1 = gtk_spin_button_new_with_range (0, 1000000, 10);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(spin1), fsguard->limit_warning);
    spin2 = gtk_spin_button_new_with_range (0, 1000000, 10);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(spin2), fsguard->limit_urgent);

    g_signal_connect (ent1, "changed", G_CALLBACK(fsguard_ent1_changed), fsguard);
    g_signal_connect (ent2, "changed", G_CALLBACK(fsguard_ent2_changed), fsguard);
    g_signal_connect (ent3, "changed", G_CALLBACK(fsguard_ent3_changed), fsguard);
    g_signal_connect (spin1, "value-changed", G_CALLBACK(fsguard_spin1_changed), fsguard);
    g_signal_connect (spin2, "value-changed", G_CALLBACK(fsguard_spin2_changed), fsguard);
    g_signal_connect (check1, "toggled", G_CALLBACK(fsguard_check1_changed), fsguard);
    g_signal_connect (check2, "toggled", G_CALLBACK(fsguard_check2_changed), fsguard);

    gtk_box_pack_start (GTK_BOX(vbox1), lab1, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox1), lab2, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox1), lab5, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox1), lab3, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox1), lab4, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox1), lab6, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox1), lab7, TRUE, FALSE, 1);

    gtk_box_pack_start (GTK_BOX(vbox2), ent1, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox2), ent2, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox2), ent3, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox2), spin1, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox2), spin2, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox2), check1, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox2), check2, TRUE, FALSE, 1);

    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox,
                        TRUE, TRUE, 0);

    gtk_widget_show_all (dlg);
}

// }}}

// initialization {{{
static void
fsguard_construct (XfcePanelPlugin *plugin)
{
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
 
    FsGuard *fsguard = fsguard_new (plugin);
    fsguard_check_fs (fsguard);
    fsguard->timeout =
      g_timeout_add (8192, (GSourceFunc) fsguard_check_fs, fsguard);

    gtk_container_add (GTK_CONTAINER (plugin), fsguard->ebox);

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
                      "orientation-changed",
                      G_CALLBACK (fsguard_set_orientation),
                      fsguard);
    g_signal_connect (plugin,
                      "configure-plugin",
                      G_CALLBACK (fsguard_create_options),
                      fsguard);

    xfce_panel_plugin_menu_show_configure (plugin);
}

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (fsguard_construct);

// }}}

// vim600: set foldmethod=marker: foldmarker={{{,}}}
