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

#include <gtk/gtk.h>

#include <libxfce4util/i18n.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <panel/plugins.h>
#include <panel/xfce.h>
#include <panel/xfce_support.h>
#include <libxfcegui4/netk-screen.h>
#include "icons.h"

#define TINY 0
#define SMALL 1
#define MEDIUM 2
#define LARGE 3

#define ICONSIZETINY 16
#define ICONSIZESMALL 32
#define ICONSIZEMEDIUM 48
#define ICONSIZELARGE 60


// }}}

// struct {{{

typedef struct
{
    GtkWidget	    *fs;
    GtkWidget       *box;
    GtkWidget	    *ebox;
    GtkWidget       *lab;
    gboolean        seen;
    gint            size;
    gint            timeout;
    gint            yellow;
    gint            red;
    gchar           *label;
    gchar           *mnt;
} gui;

static GtkTooltips *tooltips = NULL;

// }}}

// all functions {{{

static void
plugin_recreate_gui (gpointer data)
{
    gui *plugin = data;
    if (plugin->label != NULL) {
        if (plugin->lab == NULL) {
            plugin->lab = gtk_label_new (plugin->label);
	    gtk_widget_show (plugin->lab);
            gtk_box_pack_start (GTK_BOX(plugin->box), plugin->lab, FALSE, FALSE, 1);
	    gtk_box_reorder_child (GTK_BOX(plugin->box), plugin->lab, 0);
	} else {
	    if (gtk_label_get_text (GTK_LABEL(plugin->lab)) != plugin->label) {
	        gtk_label_set_text (GTK_LABEL(plugin->lab), plugin->label);
	    }
	}
    } else {
        g_object_unref (plugin->lab);
    }
}

static void
plugin_open_xffm (GtkWidget *widget, gpointer user_data)
{
    gui *plugin = user_data;
    GString *cmd;
    cmd = g_string_new ("xffm ");
    if (plugin->mnt != NULL && (strcmp(plugin->mnt, ""))) {
        g_string_append (cmd, plugin->mnt);
    }
    exec_cmd (cmd->str, FALSE, FALSE);
    g_string_free (cmd, TRUE);
}

static gboolean
plugin_check_fs (gpointer data)
{
    GdkPixbuf *pb;
    GString *tool;
    int size;
    int err;
    static struct statfs fsd;
    char msg[100];
    gui *plugin = data;

    err = statfs (plugin->mnt, &fsd);
    if (err != -1) {
        size = fsd.f_bavail * fsd.f_bsize / 1048576;
        if (size <= plugin->red) {
            pb = gdk_pixbuf_new_from_inline (sizeof(icon_red), icon_red, TRUE, NULL);
	    if (!plugin->seen) {
                if (plugin->label != NULL && (strcmp(plugin->label,""))) {
                    xfce_warn (_("Only %i MB space left on %s (%s)!"), size, plugin->mnt, plugin->label);
                } else {
                    xfce_warn (_("Only %i MB space left on %s!"), size, plugin->mnt);
		}
		plugin->seen = TRUE;
	    }
        } else if (size >= plugin->red && size <= plugin->yellow) {
            pb = gdk_pixbuf_new_from_inline (sizeof(icon_yellow), icon_yellow, TRUE, NULL);
        } else {
            pb = gdk_pixbuf_new_from_inline (sizeof(icon_green), icon_green, TRUE, NULL);
        }
        pb = gdk_pixbuf_scale_simple (pb, plugin->size, plugin->size, GDK_INTERP_BILINEAR);
        if (plugin->label != NULL && (strcmp(plugin->label,""))) {
            g_snprintf (msg, 99, _("%i MB free on %s (%s)"), size, plugin->mnt, plugin->label);
        } else if (plugin->mnt != NULL && (strcmp(plugin->mnt, ""))) {
            g_snprintf (msg, 99, _("%i MB free on %s"), size, plugin->mnt);
        } 
        gtk_tooltips_set_tip (tooltips, plugin->fs, msg, NULL);
    } else {
        pb = gdk_pixbuf_new_from_inline (sizeof(icon_unknown), icon_unknown, TRUE, NULL);
        pb = gdk_pixbuf_scale_simple (pb, plugin->size, plugin->size, GDK_INTERP_BILINEAR);
    }
       
    xfce_iconbutton_set_pixbuf(XFCE_ICONBUTTON(plugin->fs), pb);
    return TRUE;
}

static gui *
gui_new ()
{
    gui *plugin;
    GdkPixbuf *pb;
    tooltips = gtk_tooltips_new ();
    plugin = g_new(gui, 1);
    plugin->ebox = gtk_event_box_new();
    gtk_widget_show (plugin->ebox);
 
    plugin->box = gtk_hbox_new(FALSE, 0);
    gtk_widget_show (plugin->box);
   
    plugin->size = ICONSIZETINY;
    plugin->lab = NULL;
    plugin->label = NULL;
    plugin->mnt = NULL;
    plugin->yellow = 0;
    plugin->red = 0;
    plugin->timeout = 0;
    plugin->fs = xfce_iconbutton_new ();
    g_signal_connect (G_OBJECT(plugin->fs), "clicked", G_CALLBACK(plugin_open_xffm), plugin);
    pb = gdk_pixbuf_new_from_inline (sizeof(icon_unknown), icon_unknown, TRUE, NULL);
    pb = gdk_pixbuf_scale_simple (pb, plugin->size, plugin->size, GDK_INTERP_BILINEAR);
    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->fs), pb);
    gtk_button_set_relief (GTK_BUTTON(plugin->fs), GTK_RELIEF_NONE);
    
    gtk_container_add (GTK_CONTAINER(plugin->box), plugin->fs);
        
    gtk_container_add (GTK_CONTAINER(plugin->ebox), plugin->box);
    gtk_widget_show_all (plugin->ebox);
    plugin_check_fs ((gpointer)plugin);
    plugin->timeout = g_timeout_add_full (G_PRIORITY_DEFAULT, 8192, (GSourceFunc) plugin_check_fs, plugin, NULL);
    return(plugin);
}

static gboolean
create_plugin_control (Control *ctrl)
{
    gui *plugin = gui_new();

    gtk_container_add (GTK_CONTAINER(ctrl->base), plugin->ebox);

    ctrl->data = (gpointer)plugin;
    ctrl->with_popup = FALSE;

    gtk_widget_set_size_request (ctrl->base, -1, -1);

    return(TRUE);
}

static void
plugin_free (Control *ctrl)
{
    g_return_if_fail (ctrl != NULL);
    g_return_if_fail (ctrl->data != NULL);

    gui *plugin = ctrl->data;
    if (plugin->timeout != 0) {
        g_source_remove (plugin->timeout);
    }
    g_free(plugin);
}

static void
plugin_attach_callback (Control *ctrl, const gchar *signal, GCallback cb, gpointer data)
{
    gui *plugin = ctrl->data;
    g_signal_connect (plugin->ebox, signal, cb, data);
    g_signal_connect (plugin->fs, signal, cb, data);
}

static void
plugin_set_size (Control *ctrl, int size)
{
    gui *plugin = ctrl->data;
    if (size == TINY) {
        plugin->size = ICONSIZETINY;
    } else if (size == SMALL) {
        plugin->size = ICONSIZESMALL;
    } else if (size == MEDIUM) {
        plugin->size = ICONSIZEMEDIUM;
    } else {
        plugin->size = ICONSIZELARGE;
    }	
    plugin_check_fs (plugin);
}

static void
plugin_read_config (Control *ctrl, xmlNodePtr node)
{
    xmlChar *value;
    gui *plugin = ctrl->data;

    for (node = node->children; node; node = node->next) {
        if (xmlStrEqual(node->name, (const xmlChar *)"Fsguard")) {
            if ((value = xmlGetProp(node, (const xmlChar *)"yellow"))) {
                plugin->yellow = atoi(value);
                g_free(value);
            }
            if ((value = xmlGetProp(node, (const xmlChar *)"red"))) {
                plugin->red = atoi(value);
                g_free(value);
            }
            if ((value = xmlGetProp(node, (const xmlChar *)"label"))) {
                plugin->label = g_strdup((gchar *)value);
                g_free(value);
            }
            if ((value = xmlGetProp(node, (const xmlChar *)"mnt"))) {
                plugin->mnt = g_strdup((gchar *)value);
                g_free(value);
            }
            break;
	}
    }
    plugin_recreate_gui (plugin);
    plugin_check_fs (plugin);
    plugin->seen = FALSE;
}

static void
plugin_write_config (Control *ctrl, xmlNodePtr parent)
{
    gui *plugin = ctrl->data;
    xmlNodePtr root;
    char value[20];

    root = xmlNewTextChild(parent, NULL, "Fsguard", NULL);

    g_snprintf(value, sizeof(plugin->red), "%d", plugin->red);
    xmlSetProp(root, "red", value);

    g_snprintf(value, sizeof(plugin->yellow), "%d", plugin->yellow);
    xmlSetProp(root, "yellow", value);

    xmlSetProp(root, "label", plugin->label);

    xmlSetProp(root, "mnt", plugin->mnt);
}    

static void
plugin_ent1_changed (GtkWidget *widget, gpointer user_data)
{
    gui *plugin = user_data;
    plugin->label = g_strdup (gtk_entry_get_text (GTK_ENTRY(widget)));
    plugin_recreate_gui (plugin);
}

static void
plugin_ent2_changed (GtkWidget *widget, gpointer user_data)
{
    gui *plugin = user_data;
    plugin->mnt = g_strdup (gtk_entry_get_text (GTK_ENTRY(widget)));
    plugin->seen = FALSE;
    plugin_check_fs (plugin);
}

static void
plugin_spin1_changed (GtkWidget *widget, gpointer user_data)
{
    gui *plugin = user_data;
    plugin->red = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(widget));
    plugin->seen = FALSE;
}

static void
plugin_spin2_changed (GtkWidget *widget, gpointer user_data)
{
    gui *plugin = user_data;
    plugin->yellow = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(widget));
}

static void
plugin_create_options (Control *ctrl, GtkContainer *con, GtkWidget *done)
{
    gui *plugin = ctrl->data;
    GtkWidget *hbox, *vbox1, *vbox2, *mnt, *spin1, *spin2;
    GtkWidget *lab1, *lab2, *lab3, *lab4, *ent1, *ent2;
    gchar *text[] = {
	     _("label"),
	     _("mountpoint"),
             _("high alarm limit (MB)"),
	     _("high warn limit (MB)"),
    };
 
    hbox = gtk_hbox_new (FALSE, 2);
    vbox1 = gtk_vbox_new (FALSE, 5);
    vbox2 = gtk_vbox_new (FALSE, 5);

    gtk_box_pack_start (GTK_BOX(hbox), vbox1, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(hbox), vbox2, TRUE, FALSE, 1);

    lab1 = gtk_label_new (text[0]);
    lab2 = gtk_label_new (text[1]);
    lab3 = gtk_label_new (text[2]);
    lab4 = gtk_label_new (text[3]);
    ent1 = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY(ent1), 16);
    if (plugin->label != NULL) {
        gtk_entry_set_text (GTK_ENTRY(ent1), plugin->label);
    }
    ent2 = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY(ent2), 16);
    if (plugin->mnt != NULL) {
        gtk_entry_set_text (GTK_ENTRY(ent2), plugin->mnt);
    }
    spin1 = gtk_spin_button_new_with_range (0, 100000, 10);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(spin1), plugin->red);
    spin2 = gtk_spin_button_new_with_range (0, 100000, 10);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(spin2), plugin->yellow);
    g_signal_connect (ent1, "changed", G_CALLBACK(plugin_ent1_changed), plugin);
    g_signal_connect (ent2, "changed", G_CALLBACK(plugin_ent2_changed), plugin);
    g_signal_connect (spin1, "value-changed", G_CALLBACK(plugin_spin1_changed), plugin);
    g_signal_connect (spin2, "value-changed", G_CALLBACK(plugin_spin2_changed), plugin);

    gtk_box_pack_start (GTK_BOX(vbox1), lab1, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox1), lab2, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox1), lab3, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox1), lab4, TRUE, FALSE, 1);

    gtk_box_pack_start (GTK_BOX(vbox2), ent1, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox2), ent2, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox2), spin1, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox2), spin2, TRUE, FALSE, 1);

    gtk_container_add (GTK_CONTAINER(con), hbox);
    gtk_widget_show_all (hbox);
}

// }}}

// initialization {{{
G_MODULE_EXPORT void
xfce_control_class_init(ControlClass *cc)
{
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
    /* these are required */
    cc->name		= "fsguard";
    cc->caption		= _("Free Space Checker");

    cc->create_control	= (CreateControlFunc)create_plugin_control;

    cc->free		= plugin_free;
    cc->attach_callback	= plugin_attach_callback;

    /* options; don't define if you don't have any ;) */
    cc->read_config	= plugin_read_config;
    cc->write_config	= plugin_write_config;

    /* Don't use this function at all if you want xfce to
     * do the sizing.
     * Just define the set_size function to NULL, or rather, don't 
     * set it to something else.
    */
    cc->set_size		= plugin_set_size;
    cc->create_options          = plugin_create_options;
}

/* required! defined in panel/plugins.h */
XFCE_PLUGIN_CHECK_INIT

// }}}

// vim600: set foldmethod=marker: foldmarker={{{,}}}
