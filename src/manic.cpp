/***************************************************************************
 *   Copyright (C) 2009 by Marc Lajoie                                     *
 *   quickhand@openinkpot.org                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <stdio.h>
#include <string.h>

#include <Evas.h>
#include <Ecore.h>
#include <Edje.h>
#include <Elementary.h>
#include <EWebKit.h>

#define REL_THEME "themes/themes_oitheme.edj"

using namespace std;


Evas *evas;
Evas_Object *win, *bg, *mainscroll,*pagelayout,*webkitobj,*address_entry;
/* return value of app - keep it here so we can modify it */
static int retval = -1;
static unsigned char mouseclicked=0;
static unsigned char cancelmouseup=0;
int winwidth=600;
int winheight=800;

char *get_theme_file()
{
 	//char *cwd = get_current_dir_name();
	char *rel_theme;
	asprintf(&rel_theme, "%s/%s", "/usr/share/manic-browser", REL_THEME);
    //asprintf(&rel_theme, "%s/%s",cwd, REL_THEME);
	//free(cwd);
	return rel_theme;
}

const char *get_cur_url()
{
    Evas_Object *webpage=ewk_webview_object_webpage_get(webkitobj);
    if(webpage==NULL)
        return NULL;
    
    Evas_Object *frame=ewk_webpage_object_mainframe_get(webpage);
    if(frame==NULL)
    {
        return NULL;
    }
    
    return ewk_webframe_object_url_get(frame);
    
    
    
}


/* if someone presses the close button on our window - exit nicely */
static void
win_del(void *data, Evas_Object *obj, void *event_info)
{
   retval = -1;
   /* cleanly exit */
   elm_exit();
}

static void address_entry_activated(void *data, Evas_Object *obj, void *event_info)
{
    char *entrytxt=strdup(elm_entry_entry_get(address_entry));
    ewk_webview_object_load_url(webkitobj,strtok(entrytxt,"<"));
    
    free(entrytxt);
}
static void back_bt_clicked(void *data, Evas_Object *obj, void *event_info)
{
    if(ewk_webview_object_navigation_can_go_back(webkitobj))
        ewk_webview_object_navigation_back(webkitobj);
}
static void forward_bt_clicked(void *data, Evas_Object *obj, void *event_info)
{
    if(ewk_webview_object_navigation_can_go_forward(webkitobj))
        ewk_webview_object_navigation_forward(webkitobj);
}
static void reload_bt_clicked(void *data, Evas_Object *obj, void *event_info)
{
    ewk_webview_object_navigation_reload(webkitobj);
}
static void stop_bt_clicked(void *data, Evas_Object *obj, void *event_info)
{
    ewk_webview_object_navigation_stop(webkitobj);
}
static void content_resize()
{
    if(!evas_object_visible_get(webkitobj))
        return;
    
    Evas_Object *webpage=ewk_webview_object_webpage_get(webkitobj);
    if(webpage==NULL)
        return;
    if(!evas_object_visible_get(webpage))
        return;
    
    Evas_Object *frame=ewk_webpage_object_mainframe_get(webpage);
    if(frame==NULL)
    {
        return;
    }
    
    int framew,frameh;
    ewk_webframe_object_contents_size_get(frame,&framew,&frameh);
    if(framew>0 && frameh>0)
        evas_object_resize(pagelayout,framew-1,frameh-1);
}

static void  mainscroll_resizing(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    content_resize();
}
static void  webkitobj_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    cancelmouseup=0;
    mouseclicked=1;
    ewk_event_feed_mouse_down(webkitobj, (Evas_Event_Mouse_Down *)event_info);
}
static void  webkitobj_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    if(!mouseclicked)
        ewk_event_feed_mouse_move(webkitobj, (Evas_Event_Mouse_Move *)event_info);
    else
        cancelmouseup=1;
}
static void  webkitobj_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    
    mouseclicked=0;
    if(!cancelmouseup)
        ewk_event_feed_mouse_up(webkitobj, (Evas_Event_Mouse_Up *)event_info);
    cancelmouseup=0;
    evas_object_focus_set(webkitobj,TRUE);
}
static void  webkitobj_key_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    ewk_event_feed_key_press(webkitobj, (Evas_Event_Key_Down *)event_info);
}

static void page_loading(void *data, Evas_Object *obj, void *event_info)
{
    
    content_resize();
}


static void page_url_changed(void *data, Evas_Object *obj, void *event_info)
{
    int x,y,w,h;
    edje_object_part_text_cursor_geometry_get(mainscroll,"bg",&x,&y,&w,&h);
    //evas_object_geometry_get(mainscroll,&x,&y,&w,&h);
    evas_object_resize(pagelayout,w,h);
    elm_entry_entry_set(address_entry,get_cur_url());
    
}



EAPI int
elm_main(int argc, char **argv)
{
   Evas_Object *bx, *bx2, *bt;
   
   /* new window - do the usual and give it a name, title and delete handler */
   win = elm_win_add(NULL, "dialog", ELM_WIN_BASIC);
   elm_win_title_set(win, "Dialog");
   evas_object_smart_callback_add(win, "delete-request", win_del, NULL);
   evas_object_resize(win,winwidth,winheight);
   evas_object_size_hint_max_set(win,winwidth,winheight);
   /*get evas*/
   evas=evas_object_evas_get(win);
   ewk_init(evas);
   char *themefile;
   themefile=get_theme_file();
   ewk_theme_set(themefile);
   /* add a standard bg */
   bg = elm_bg_add(win);
   /* not not allow bg to expand. let's limit dialog size to contents */
   evas_object_size_hint_weight_set(bg, 1.0, 1.0);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   /* add a box object - default is vertical. a box holds children in a row,
    * either horizontally or vertically. nothing more. */
   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, 1.0, 1.0);
   evas_object_size_hint_align_set(bt, -1.0, -1.0);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);
   
   /* add controls box */
   bx2 = elm_box_add(win);
   elm_box_horizontal_set(bx2,TRUE);
   evas_object_size_hint_weight_set(bx2, 1.0, 0.0);
   //evas_object_size_hint_weight_set(mainscroll, 1.0, 1.0);
   evas_object_size_hint_align_set(mainscroll, -1.0, -1.0);
   elm_box_homogenous_set(bx2,FALSE);
   elm_box_pack_end(bx,bx2);
   evas_object_show(bx2);
   
   /* add buttons to control box */
   
   bt = elm_button_add(win);
   elm_button_label_set(bt, "B");
   evas_object_size_hint_weight_set(bt,0.0,0.0);
   //evas_object_size_hint_align_set(bt, -1.0, -1.0);
   elm_box_pack_end(bx2, bt);
   evas_object_smart_callback_add(bt,"clicked",back_bt_clicked, NULL);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_button_label_set(bt, "F");
   evas_object_size_hint_weight_set(bt,0.0,0.0);
   //evas_object_size_hint_align_set(bt, -1.0, -1.0);
   elm_box_pack_end(bx2, bt);
   evas_object_smart_callback_add(bt,"clicked",forward_bt_clicked, NULL);
   evas_object_show(bt);
   
   bt = elm_button_add(win);
   elm_button_label_set(bt, "R");
   evas_object_size_hint_weight_set(bt, 0.0, 0.0);
   //evas_object_size_hint_align_set(bt, -1.0, -1.0);
   elm_box_pack_end(bx2, bt);
   evas_object_smart_callback_add(bt,"clicked",reload_bt_clicked, NULL);
   evas_object_show(bt);
   
   bt = elm_button_add(win);
   elm_button_label_set(bt, "S");
   evas_object_size_hint_weight_set(bt, 0.0, 0.0);
   //evas_object_size_hint_align_set(bt, -1.0, -1.0);
   elm_box_pack_end(bx2, bt);
   evas_object_smart_callback_add(bt,"clicked",stop_bt_clicked, NULL);
   evas_object_show(bt);
   
   Evas_Object *entry_frame=elm_frame_add(win);
   evas_object_size_hint_weight_set(entry_frame, 1.0, 1.0);
   evas_object_size_hint_align_set(entry_frame, -1.0, -1.0);
   elm_frame_style_set(entry_frame,"default");
   elm_box_pack_end(bx2, entry_frame);
   
   address_entry=elm_entry_add(win);
   elm_entry_single_line_set(address_entry,TRUE);
   evas_object_size_hint_weight_set(address_entry, 0.0, 1.0);
   evas_object_size_hint_align_set(address_entry, -1.0, 1.0);
   elm_entry_entry_set(address_entry,"http://www.openinkpot.org");
   evas_object_smart_callback_add(address_entry, "activated", address_entry_activated, NULL);
   elm_frame_content_set(entry_frame,address_entry);
   evas_object_show(address_entry);
   evas_object_show(entry_frame);
   
   
   /* add scroller that will house main content */
   
   mainscroll=elm_scroller_add(win);
   evas_object_size_hint_weight_set(mainscroll, 1.0, 1.0);
   evas_object_size_hint_align_set(mainscroll, -1.0, -1.0);
   elm_box_pack_end(bx,mainscroll);
   evas_object_event_callback_add(mainscroll,EVAS_CALLBACK_RESIZE,mainscroll_resizing,NULL);
   evas_object_show(mainscroll);
   
   int scrx,scry,scrw,scrh;
   evas_object_geometry_get(mainscroll,&scrx,&scry,&scrw,&scrh);
   //elm_scroller_region_show(mainscroll,0, 0,scrw,scrh);
   
   /* set up layout */
   
   pagelayout=elm_layout_add(win);
   elm_layout_file_set(pagelayout,themefile,"pagelayout");
   free(themefile);
   elm_scroller_content_set(mainscroll,pagelayout);
   evas_object_show(pagelayout);
   
   /* add webkit component to layout */
   
   webkitobj = ewk_webview_object_add(evas);
   
   elm_layout_content_set(pagelayout,"pagelayout/swallow",webkitobj);
   
   ewk_webview_object_load_url(webkitobj, "http://www.openinkpot.org");
   
   ewk_callback_load_progress_add(webkitobj,page_loading,NULL);
   ewk_callback_url_changed_add(webkitobj,page_url_changed,NULL);
   evas_object_event_callback_add(webkitobj,EVAS_CALLBACK_MOUSE_DOWN,webkitobj_mouse_down,NULL);
   evas_object_event_callback_add(webkitobj,EVAS_CALLBACK_MOUSE_MOVE,webkitobj_mouse_move,NULL);
   evas_object_event_callback_add(webkitobj,EVAS_CALLBACK_MOUSE_UP,webkitobj_mouse_up,NULL);
   evas_object_event_callback_add(webkitobj,EVAS_CALLBACK_KEY_DOWN,webkitobj_key_down,NULL);
   
   
   evas_object_show(webkitobj);
   
   /* show the window */
   evas_object_show(win);



   elm_run();
   elm_shutdown();
   ewk_shutdown();
   return 0;
}
ELM_MAIN()
