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
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/param.h>
#include <sys/mount.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "icons.h"

#define HORIZONTAL 0
#define VERTICAL 1

#define TINY 0
#define SMALL 1
#define MEDIUM 2
#define LARGE 3

#define ICONSIZETINY 24 
#define ICONSIZESMALL 30
#define ICONSIZEMEDIUM 45
#define ICONSIZELARGE 60

#define BORDER 8

// }}}

// struct {{{

typedef struct
{
    XfcePanelPlugin *plugin;

    GtkWidget	    *fs;
    GtkWidget       *box;
    GtkWidget	    *ebox;
    GtkWidget       *lab;
    gboolean        seen;
    gint            size;
    gint            timeout;
    gint            yellow;
    gint            red;
    GtkOrientation  orientation;
    gchar           *label;
    gchar           *mnt;
    gchar 	    *filemanager;
} FsGuard;

static GtkTooltips *tooltips = NULL;

// }}}

// all functions {{{

static void
fsguard_recreate_gui (gpointer data)
{
    FsGuard *fsguard = data;
    if (fsguard->label != NULL && (strlen (fsguard->label) > 0)) {
        if (fsguard->lab == NULL) {
            fsguard->lab = gtk_label_new (fsguard->label);
	    gtk_widget_show (fsguard->lab);
            gtk_box_pack_start (GTK_BOX(fsguard->box), fsguard->lab, FALSE, FALSE, 0);
	    gtk_box_reorder_child (GTK_BOX(fsguard->box), fsguard->lab, 0);
	} 

        gtk_label_set_text (GTK_LABEL(fsguard->lab), fsguard->label);
    } else {
        if (GTK_IS_WIDGET (fsguard->lab)) {
            gtk_widget_destroy (fsguard->lab);
	    fsguard->lab = NULL;
	}
    }
}

static void
fsguard_open_mnt (GtkWidget *widget, gpointer user_data)
{
    FsGuard *fsguard = user_data;
    GString *cmd;
    if (strlen(fsguard->filemanager) == 0) {
        return;
    }
    cmd = g_string_new (fsguard->filemanager);
    if (fsguard->mnt != NULL && (strcmp(fsguard->mnt, ""))) {
        g_string_append (cmd, " ");
        g_string_append (cmd, fsguard->mnt);
    }
    xfce_exec (cmd->str, FALSE, FALSE, NULL);
    g_string_free (cmd, TRUE);
}

static gboolean
fsguard_check_fs (gpointer data)
{
    GdkPixbuf *pb;
    float size = 0;
    float freeblocks = 0;
    long blocksize;
    int err;
    gchar msg[100];
    static struct statfs fsd;
    FsGuard *fsguard = data;

    err = statfs (fsguard->mnt, &fsd);
    
    if (err != -1) {
        blocksize = fsd.f_bsize;
        freeblocks = fsd.f_bavail;
        size = (freeblocks * blocksize) / 1048576;
        if (size <= fsguard->red) {
            pb = gdk_pixbuf_new_from_inline (sizeof(icon_red), icon_red, FALSE, NULL);
	    if (!fsguard->seen) {
                if (fsguard->label != NULL && (strcmp(fsguard->label,"")) && (strcmp(fsguard->mnt, fsguard->label))) {
		    if (size > 1024) {
                        xfce_warn (_("Only %.2f GB space left on %s (%s)!"), size/1024, fsguard->mnt, fsguard->label);
		    } else {
                        xfce_warn (_("Only %.2f MB space left on %s (%s)!"), size, fsguard->mnt, fsguard->label);
		    }
                } else {
		    if (size > 1024) {
                        xfce_warn (_("Only %.2f GB space left on %s!"), size/1024, fsguard->mnt);
		    } else {
                        xfce_warn (_("Only %.2f MB space left on %s!"), size, fsguard->mnt);
		    }
		}
		fsguard->seen = TRUE;
	    }
        } else if (size >= fsguard->red && size <= fsguard->yellow) {
            pb = gdk_pixbuf_new_from_inline (sizeof(icon_yellow), icon_yellow, FALSE, NULL);
        } else {
            pb = gdk_pixbuf_new_from_inline (sizeof(icon_green), icon_green, FALSE, NULL);
        }
        if (fsguard->label != NULL && (strcmp(fsguard->label,"")) && (strcmp(fsguard->mnt, fsguard->label))) {
	    if (size > 1024) {
                g_snprintf (msg, sizeof (msg), _("%.2f GB space left on %s (%s)"), size/1024, fsguard->mnt, fsguard->label);
	    } else {
                g_snprintf (msg, sizeof (msg), _("%.2f MB space left on %s (%s)"), size, fsguard->mnt, fsguard->label);
	    }
        } else if (fsguard->mnt != NULL && (strcmp(fsguard->mnt, ""))) {
	    if (size > 1024) {
                g_snprintf (msg, sizeof (msg), _("%.2f GB space left on %s"), size/1024, fsguard->mnt);
	    } else {
                g_snprintf (msg, sizeof (msg), _("%.2f MB space left on %s"), size, fsguard->mnt);
	    }
        } 
    } else {
        pb = gdk_pixbuf_new_from_inline (sizeof(icon_unknown), icon_unknown, FALSE, NULL);
        g_snprintf (msg, sizeof (msg), _("could not check mountpoint %s, please check your config"), fsguard->mnt);
    }
    
    gtk_tooltips_set_tip (tooltips, fsguard->fs, msg, NULL);
       
    xfce_iconbutton_set_pixbuf(XFCE_ICONBUTTON(fsguard->fs), pb);
    g_object_unref (G_OBJECT(pb));
    return TRUE;
}

static FsGuard *
create_fsguard_control (XfcePanelPlugin *plugin)
{
    FsGuard *fsguard;
    tooltips = gtk_tooltips_new ();
    fsguard = g_new(FsGuard, 1);

    fsguard->plugin = plugin;
    fsguard->orientation = xfce_panel_plugin_get_orientation(plugin);
    
    fsguard->ebox = gtk_event_box_new();
    gtk_widget_show (fsguard->ebox);

    if (fsguard->orientation == GTK_ORIENTATION_VERTICAL) {
	fsguard->box = gtk_vbox_new (FALSE, 2);
    } else {
	fsguard->box = gtk_hbox_new (FALSE, 2);
    }
    gtk_widget_show (fsguard->box);

    fsguard->fs = xfce_iconbutton_new ();
    gtk_widget_show (fsguard->fs);
    g_signal_connect (G_OBJECT(fsguard->fs), "clicked", G_CALLBACK(fsguard_open_mnt), fsguard);
    
    gtk_button_set_relief (GTK_BUTTON(fsguard->fs), GTK_RELIEF_NONE);
    gtk_container_add (GTK_CONTAINER(fsguard->box), fsguard->fs);
    gtk_container_add (GTK_CONTAINER(fsguard->ebox), fsguard->box);

    xfce_panel_plugin_add_action_widget (plugin, fsguard->ebox);
    xfce_panel_plugin_add_action_widget (plugin, fsguard->fs);
    
    fsguard->size = ICONSIZETINY;
    fsguard->lab = NULL;
    fsguard->label = NULL;
    fsguard->mnt = NULL;
    fsguard->yellow = 0;
    fsguard->red = 0;
    fsguard->timeout = 0;
    fsguard->filemanager = NULL;
    
    return fsguard;
}

static void
fsguard_free (XfcePanelPlugin *plugin, FsGuard *fsguard)
{
    if (fsguard->timeout != 0) {
        g_source_remove (fsguard->timeout);
    }

    g_free (fsguard->label);
    g_free (fsguard->mnt);
    g_free (fsguard->filemanager);

    g_free(fsguard);
}

static void
fsguard_set_orientation (XfcePanelPlugin *plugin, GtkOrientation orientation, FsGuard *fsguard)
{
    GtkWidget* box;

    fsguard->orientation = orientation;
    if (fsguard->orientation == GTK_ORIENTATION_VERTICAL) {
	box = gtk_vbox_new (FALSE, 2);
        gtk_widget_reparent (fsguard->lab, box);
        gtk_widget_reparent (fsguard->fs, box);
    } else {
        box = gtk_hbox_new (FALSE, 2);
        gtk_widget_reparent (fsguard->lab, box);
        gtk_widget_reparent (fsguard->fs, box);
    }
    gtk_widget_destroy (fsguard->box);
    fsguard->box = box;
    gtk_container_add (GTK_CONTAINER(fsguard->ebox), fsguard->box);
    gtk_widget_show (fsguard->box);
}

static gboolean
fsguard_set_size (XfcePanelPlugin *plugin, int size, FsGuard *fsguard)
{
    fsguard->size = size;
    fsguard_check_fs (fsguard);
    gtk_widget_set_size_request (fsguard->fs, fsguard->size, fsguard->size);

    return TRUE;
}

static void
fsguard_read_config (XfcePanelPlugin *plugin, FsGuard *fsguard)
{
    const char *value;
    char *file;
    XfceRc *rc;
    
    if (!(file = xfce_panel_plugin_lookup_rc_file (plugin)))
        return;
    
    rc = xfce_rc_simple_open (file, TRUE);
    g_free (file);

    if (!rc)
        return;
    
    fsguard->yellow = xfce_rc_read_int_entry (rc, "yellow", 0);

    fsguard->red = xfce_rc_read_int_entry (rc, "red", 0);

    if ((value = xfce_rc_read_entry (rc, "label", NULL)) && *value) {
        fsguard->label = g_strdup(value);

    }
    
    if ((value = xfce_rc_read_entry (rc, "mnt", NULL)) && *value) {
        fsguard->mnt = g_strdup(value);
    }
    
    if ((value = xfce_rc_read_entry (rc, "filemanager", NULL)) && *value) {
        fsguard->filemanager = g_strdup(value);
    }
    else {
        fsguard->filemanager = g_strdup ("xffm");
    }

    xfce_rc_close (rc);
}

static void
fsguard_write_config (XfcePanelPlugin *plugin, FsGuard *fsguard)
{
    XfceRc *rc;
    char *file;

    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
        return;
    
    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc)
        return;
    
    xfce_rc_write_int_entry (rc, "yellow", fsguard->yellow);
    
    xfce_rc_write_int_entry (rc, "red", fsguard->red);
    
    xfce_rc_write_entry (rc, "label", fsguard->label ? fsguard->label : "");
    
    xfce_rc_write_entry (rc, "mnt", fsguard->mnt ? fsguard->mnt : "");
    
    xfce_rc_write_entry (rc, "filemanager", 
                         fsguard->filemanager ? fsguard->filemanager : "");

    xfce_rc_close (rc);
}    

static void
fsguard_ent1_changed (GtkWidget *widget, gpointer user_data)
{
    FsGuard *fsguard = user_data;
    fsguard->label = g_strdup (gtk_entry_get_text (GTK_ENTRY(widget)));
    fsguard_recreate_gui (fsguard);
}

static void
fsguard_ent2_changed (GtkWidget *widget, gpointer user_data)
{
    FsGuard *fsguard = user_data;
    fsguard->mnt = g_strdup (gtk_entry_get_text (GTK_ENTRY(widget)));
    fsguard->seen = FALSE;
    fsguard_check_fs (fsguard);
}

static void
fsguard_ent3_changed (GtkWidget *widget, gpointer user_data)
{
    FsGuard *fsguard = user_data;
    fsguard->filemanager = g_strdup (gtk_entry_get_text (GTK_ENTRY(widget)));
}

static void
fsguard_spin1_changed (GtkWidget *widget, gpointer user_data)
{
    FsGuard *fsguard = user_data;
    fsguard->red = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(widget));
    fsguard->seen = FALSE;
}

static void
fsguard_spin2_changed (GtkWidget *widget, gpointer user_data)
{
    FsGuard *fsguard = user_data;
    fsguard->yellow = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(widget));
}

static void
fsguard_dialog_response (GtkWidget *dlg, int response, FsGuard *fsguard)
{
    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (fsguard->plugin);
    fsguard_write_config (fsguard->plugin, fsguard);
    fsguard_recreate_gui (fsguard);
}

static void
fsguard_create_options (XfcePanelPlugin *plugin, FsGuard *fsguard)
{
    GtkWidget *dlg, *header;
    GdkPixbuf *pb;
    GtkWidget *hbox, *vbox1, *vbox2, *spin1, *spin2;
    GtkWidget *lab1, *lab2, *lab3, *lab4, *lab5, *ent1, *ent2, *ent3;
    gchar *text[] = {
	     N_("label"),
	     N_("mountpoint"),
             N_("high alarm limit (MB)"),
	     N_("high warn limit (MB)"),
	     N_("filemanager"),
    };
    xfce_panel_plugin_block_menu (plugin);
    
    dlg = gtk_dialog_new_with_buttons (_("Properties"), 
                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                GTK_DIALOG_DESTROY_WITH_PARENT |
                GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);
    
    g_signal_connect (dlg, "response", G_CALLBACK (fsguard_dialog_response),
                      fsguard);

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);
    
    pb = xfce_themed_icon_load ("xfce4-panel", 48);
    gtk_window_set_icon (GTK_WINDOW (dlg), pb);
    g_object_unref (pb);
    
    header = xfce_create_header (NULL, _("Free Space Checker"));
    gtk_widget_set_size_request (GTK_BIN (header)->child, -1, 32);
    gtk_container_set_border_width (GTK_CONTAINER (header), BORDER - 2);
    gtk_widget_show (header);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), header,
                        FALSE, TRUE, 0);
    
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

    ent1 = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY(ent1), 16);
    if (fsguard->label != NULL) {
        gtk_entry_set_text (GTK_ENTRY(ent1), fsguard->label);
    }
    ent2 = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY(ent2), 32);
    if (fsguard->mnt != NULL) {
        gtk_entry_set_text (GTK_ENTRY(ent2), fsguard->mnt);
    }
    ent3 = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY(ent3), 16);
    if (fsguard->filemanager != NULL) {
        gtk_entry_set_text (GTK_ENTRY(ent3), fsguard->filemanager);
    }
 
    spin1 = gtk_spin_button_new_with_range (0, 1000000, 10);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(spin1), fsguard->red);
    spin2 = gtk_spin_button_new_with_range (0, 1000000, 10);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(spin2), fsguard->yellow);

    g_signal_connect (ent1, "changed", G_CALLBACK(fsguard_ent1_changed), fsguard);
    g_signal_connect (ent2, "changed", G_CALLBACK(fsguard_ent2_changed), fsguard);
    g_signal_connect (ent3, "changed", G_CALLBACK(fsguard_ent3_changed), fsguard);
    g_signal_connect (spin1, "value-changed", G_CALLBACK(fsguard_spin1_changed), fsguard);
    g_signal_connect (spin2, "value-changed", G_CALLBACK(fsguard_spin2_changed), fsguard);

    gtk_box_pack_start (GTK_BOX(vbox1), lab1, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox1), lab2, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox1), lab5, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox1), lab3, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox1), lab4, TRUE, FALSE, 1);

    gtk_box_pack_start (GTK_BOX(vbox2), ent1, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox2), ent2, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox2), ent3, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox2), spin1, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox2), spin2, TRUE, FALSE, 1);

    gtk_widget_show_all (hbox);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox,
                        TRUE, TRUE, 0);

    gtk_widget_show (dlg);
}

// }}}

// initialization {{{
static void
fsguard_construct (XfcePanelPlugin *plugin)
{
    FsGuard *fsguard;
    
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
 
    fsguard = create_fsguard_control (plugin);

    fsguard_read_config (plugin, fsguard);
    
    fsguard_recreate_gui (fsguard);
    fsguard_check_fs (fsguard);
    fsguard->seen = FALSE;

    fsguard->timeout = 
        g_timeout_add (8192, (GSourceFunc) fsguard_check_fs, fsguard);

    gtk_container_add (GTK_CONTAINER (plugin), fsguard->ebox);

    g_signal_connect (plugin, "free-data", G_CALLBACK (fsguard_free), fsguard);
    
    g_signal_connect (plugin, "save", G_CALLBACK (fsguard_write_config), 
                      fsguard);
    
    g_signal_connect (plugin, "size-changed", G_CALLBACK (fsguard_set_size), 
                      fsguard);
    
    g_signal_connect (plugin, "orientation-changed", 
                      G_CALLBACK (fsguard_set_orientation), fsguard);
	
    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin", 
                      G_CALLBACK (fsguard_create_options), fsguard);
}

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (fsguard_construct);

// }}}

// vim600: set foldmethod=marker: foldmarker={{{,}}}
