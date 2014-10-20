// forms.h

// Somewhat compatible with forms library Version 2.3a by Mark Overmars

// XXX Under development, many widgets and features missing!

typedef struct form_s {
  int winid;           // window id of the form
  int w, h;            // size of the form
  int inactive;        // whether active
  int hide;            // whether being displayed
  int modal;           // wants all input    XXX setter  
  int redraw;          // needs redrawing
  uint col;            // background color    XXX setter
  void (*call)();      // callback handler
  struct ob_s *first;  // first object in the form
  struct ob_s *last;   // last object in the form
  struct ob_s *focus;  // object to which input is directed
  struct form_s *next; // next form
} FL_FORM;

typedef struct ob_s {
  int class;           // object class
  int type;            // object type
  int boxtype;         // bounding box type
  int x, y, w, h;      // bounding box
  uint col, pcol, lcol, bcol, tcol; // colors
  char label[64];      // label
  int align;           // label alignment
  int inactive;        // whether active
  int hide;            // being displayed
  int (*handle)();     // event handler
  void (*call)();      // callback handler
  int arg;             // callback argument
  struct ob_s *next;   // next object in the form
  struct ob_s *prev;   // previous object in the form
  struct form_s *form; // form to which this object belongs
  int val;             // XXX double?  XXX text, always, etc
} FL_OBJECT;

enum { FL_BEGIN_GROUP, FL_END_GROUP, FL_BOX, FL_BUTTON, FL_INPUT }; // object class
enum { FL_NO_BOX, FL_UP_BOX, FL_DOWN_BOX, FL_FLAT_BOX, FL_BORDER_BOX, FL_TAB_BOX }; // box type
enum { FL_NORMAL_TEXT }; // text type
enum { FL_NORMAL_BUTTON, FL_TOGGLE_BUTTON, FL_RADIO_BUTTON, FL_TOUCH_BUTTON }; // button type XXX toggle vs push?

// label alignment
enum {             FL_ALIGN_LEFT,        FL_ALIGN_RIGHT,
  FL_ALIGN_TOP,    FL_ALIGN_TOP_LEFT,    FL_ALIGN_TOP_RIGHT,
  FL_ALIGN_BOTTOM, FL_ALIGN_BOTTOM_LEFT, FL_ALIGN_BOTTOM_RIGHT,
  FL_ALIGN_BEFORE, FL_ALIGN_CENTER,      FL_ALIGN_AFTER, 
  FL_ALIGN_ABOVE,  FL_ALIGN_ABOVE_LEFT,  FL_ALIGN_ABOVE_RIGHT, 
  FL_ALIGN_BELOW,  FL_ALIGN_BELOW_LEFT,  FL_ALIGN_BELOW_RIGHT };

FL_FORM *fl_current_form, *fl_forms;
FL_OBJECT *fl_pushob, *fl_mouseob;

// handy colors
enum {
  FL_BLACK  = 0x000000ff, FL_SILVER  = 0xc0c0c0ff,
  FL_MAROON = 0x800000ff, FL_RED     = 0xff0000ff,
  FL_GREEN  = 0x008000ff, FL_LIME    = 0x00ff00ff,
  FL_OLIVE  = 0x808000ff, FL_YELLOW  = 0xffff00ff,
  FL_NAVY   = 0x000080ff, FL_BLUE    = 0x0000ffff,
  FL_PURPLE = 0x800080ff, FL_FUCHSIA = 0xff00ffff,
  FL_TEAL   = 0x008080ff, FL_AQUA    = 0x00ffffff,
  FL_GRAY   = 0x808080ff, FL_WHITE   = 0xffffffff
};

// default colors
uint fl_form_color   = 0xeaeaeaff;
uint fl_ob_color     = 0xeaeaeaff;
uint fl_push_color   = 0xd3d3d3ff;
uint fl_label_color  = FL_BLACK;
uint fl_border_color = 0x949494ff;
uint fl_text_color   = FL_BLACK;
uint fl_input_color  = FL_WHITE;
uint fl_focus_color  = FL_YELLOW;

int fontbase; // XXX see label()
int default_texture; // XXX likewise font specific() XXX needs work

// fudge factors
float fx1 = 0.3;
float fx2 = 0.5;
float fy1 = 0.3;
float fy2 = 0.5;

int gentexture(int w, int h, int *data)
{
  int id;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  return id;
}

void genfonts(void)
{
  int i;
  fontbase = glGenLists(96);
  glNewList(fontbase, GL_COMPILE);
  glTranslatef(6.0, 0.0, 0.0);     
  glEndList(); 
  for (i=33;i<128;i++) {
    glNewList(fontbase + i - 32, GL_COMPILE);
    glBegin(GL_QUADS);
    glTexCoord2f(6.0/256.0 * (i & 31),              12.0/256.0 * (i >> 5) + 50.0/256.0);  glVertex2f(0.0, 12.0);
    glTexCoord2f(6.0/256.0 * (i & 31),              12.0/256.0 * (i >> 5) + 62.0/256.0);  glVertex2f(0.0,  0.0);
    glTexCoord2f(6.0/256.0 * (i & 31) +  6.0/256.0, 12.0/256.0 * (i >> 5) + 62.0/256.0);  glVertex2f(6.0,  0.0);
    glTexCoord2f(6.0/256.0 * (i & 31) +  6.0/256.0, 12.0/256.0 * (i >> 5) + 50.0/256.0);  glVertex2f(6.0, 12.0);      
    glEnd();
    glTranslatef(6.0, 0.0, 0.0);     
    glEndList(); 
  }
}

void load_fonts(void)
{
  int x, y, i, j, k, b, *image;
  memset(image = malloc(256*256*4), 0, 256*256*4);
  for (i=33;i<128;i++) for (j=0;j<10;j++) for (k=1;k<6;k++) {
    x = (i & 31)*6  + k - 1;
    y = (i >> 5)*12 + j;
//  image[x+((y+51)*256)] = ((fontdata[(((i-32)>>3)*8*10) + ((i-32)&7) + j*8] >> (7-k)     ) & 1) * -1;
    image[x+((y+51)*256)] = ((fontdata[(((i-32)>>3)*8*10) + ((i-32)&7) + j*8] >> ((7-k)*3) ) & 1) * -1;
  }
  default_texture = gentexture(256,256,image);
  genfonts();
}

// drawing
void fl_color(uint col)
{
  glColor4ubv(&col);
}

void fl_rect(float x, float y, float w, float h, uint c)
{
  if (c) { fl_color(c); glBegin(GL_QUADS); glVertex2f(x, y); glVertex2f(x+w, y); glVertex2f(x+w, y+h); glVertex2f(x, y+h); glEnd(); }
}

void fl_bound(float x, float y, float w, float h, uint c)
{
  if (c) { fl_color(c); glBegin(GL_LINE_LOOP); glVertex2f(x, y); glVertex2f(x+w, y); glVertex2f(x+w, y+h); glVertex2f(x, y+h); glEnd(); }
}

void fl_draw_box(int boxtype, int x, int y, int w, int h, int c, int b)
{
  switch (boxtype) {
  case FL_BORDER_BOX:
    fl_rect(x+1, y+1, w-1, h-1, c);
    fl_bound(x+fx1, y+fy1, w-1+fx2, h-1+fy2, b);
    break;
  case FL_FLAT_BOX:
    fl_rect(x, y, w, h, c);
    break;
  case FL_UP_BOX:
    fl_rect(x+1, y+1, w-1, h-1, c);
    if (b) {
      fl_color(b);        glBegin(GL_LINE_STRIP); glVertex2f(x+fx1, y  +fy1); glVertex2f(x+w-1+fx2, y    +fy1); glVertex2f(x+w-1+fx2, y+h  +fy2); glEnd();
      fl_color(FL_WHITE); glBegin(GL_LINE_STRIP); glVertex2f(x+fx1, y+1+fy1); glVertex2f(x    +fx1, y+h-1+fy2); glVertex2f(x+w-1+fx2, y+h-1+fy2); glEnd();
    }
    break;
  case FL_DOWN_BOX:
    fl_rect(x+1, y+1, w-1, h-1, c);
    if (b) {
      fl_color(FL_WHITE); glBegin(GL_LINE_STRIP); glVertex2f(x+fx1, y  +fy1); glVertex2f(x+w-1+fx2, y    +fy1); glVertex2f(x+w-1+fx2, y+h  +fy2); glEnd();
      fl_color(b);        glBegin(GL_LINE_STRIP); glVertex2f(x+fx1, y+1+fy1); glVertex2f(x    +fx1, y+h-1+fy2); glVertex2f(x+w-1+fx2, y+h-1+fy2); glEnd();
    }
    break;
  case FL_TAB_BOX:
    fl_rect(x+1, y, w-1, h-1, c);
    if (b) {
      fl_color(b); glBegin(GL_LINE_STRIP); 
      glVertex2f(x+fx1, y+fy1); glVertex2f(x+fx1, y+h-1+fy2); glVertex2f(x+w-1+fx2, y+h-1+fy2); glVertex2f(x+w-1+fx2, y+fy1);
      glEnd();
    }
    break;
  }
}

void fl_draw_text(int x, int y, char *s)
{
  glPushMatrix();
  glTranslatef(x, y, 0.0); 
  glEnable(GL_TEXTURE_2D);
  glListBase(fontbase - 32);
  glCallLists(strlen(s), GL_UNSIGNED_BYTE, s);
  glDisable(GL_TEXTURE_2D);
  glPopMatrix();
}

void fl_draw_label(int align, int x, int y, int w, int h, char *s)
{
  int hh, ww, gap, slen;
  if (!(slen = strlen(s))) return; // XXX compute if not fixed
  gap = 2;  // XXX globals based on font
  hh = 12;  // XXX globals based on font   push/pop font:  old = font(x) ... font(old);
  ww = slen * 6;
  switch (align) {
  case FL_ALIGN_TOP_LEFT:    case FL_ALIGN_LEFT:  case FL_ALIGN_BOTTOM_LEFT:  x += gap+1; break;
  case FL_ALIGN_TOP_RIGHT:   case FL_ALIGN_RIGHT: case FL_ALIGN_BOTTOM_RIGHT: x += w-ww-gap; break;
  case FL_ALIGN_ABOVE:       case FL_ALIGN_TOP:   case FL_ALIGN_CENTER:
  case FL_ALIGN_BOTTOM:      case FL_ALIGN_BELOW:        x += (w-ww)/2+1; break;
  case FL_ALIGN_BEFORE:                                  x -= ww+gap; break;
  case FL_ALIGN_AFTER:                                   x += w+gap; break;
  case FL_ALIGN_ABOVE_RIGHT: case FL_ALIGN_BELOW_RIGHT:  x += w-ww;
  }
  switch (align) {
  case FL_ALIGN_ABOVE_LEFT:  case FL_ALIGN_ABOVE:  case FL_ALIGN_ABOVE_RIGHT:  y += h; break;
  case FL_ALIGN_TOP_LEFT:    case FL_ALIGN_TOP:    case FL_ALIGN_TOP_RIGHT:    y += h-hh-gap; break;
  case FL_ALIGN_BEFORE:      case FL_ALIGN_LEFT:   case FL_ALIGN_CENTER:
  case FL_ALIGN_RIGHT:       case FL_ALIGN_AFTER:                              y += (h-hh-gap)/2; break;
  case FL_ALIGN_BOTTOM_LEFT: case FL_ALIGN_BOTTOM: case FL_ALIGN_BOTTOM_RIGHT: y += gap-1; break;
  case FL_ALIGN_BELOW_LEFT:  case FL_ALIGN_BELOW:  case FL_ALIGN_BELOW_RIGHT:  y -= hh+gap-1; break;
  }  
  fl_draw_text(x, y, s);
}

// objects
FL_OBJECT *fl_make_object(int class, int type, int x, int y, int w, int h, char *label, int (*handle)())
{
  FL_OBJECT *ob = (FL_OBJECT *) memset(malloc(sizeof(FL_OBJECT)), 0, sizeof(FL_OBJECT));
  ob->class = class;
  ob->type = type;
  ob->x = x; ob->y = y; ob->w = w; ob->h = h;
  if (label) strcpy(ob->label, label);
  ob->align = FL_ALIGN_CENTER;
  ob->col  = fl_ob_color;
  ob->pcol = fl_push_color;
  ob->lcol = fl_label_color;
  ob->bcol = fl_border_color;
  ob->tcol = fl_text_color;
  ob->handle = handle;
  return ob;
}

void fl_add_object(FL_FORM *form, FL_OBJECT *ob)
{
  if (ob->prev = form->last) ob->prev->next = ob; else form->first = ob;
  ob->next = 0;
  ob->form = form;
  form->last = ob;
  form->redraw = 1;
}

void fl_delete_object(FL_OBJECT *ob)
{
  if (ob->form->focus == ob) ob->form->focus = 0;
  if (ob->prev) ob->prev->next = ob->next; else ob->form->first = ob->next;
  if (ob->next) ob->next->prev = ob->prev; else ob->form->last  = ob->prev;
  ob->form->redraw = 1;
  ob->form = 0;
}

void fl_set_object_boxtype(FL_OBJECT *ob, int boxtype)
{
  if (ob->boxtype != boxtype) { ob->boxtype = boxtype; if (ob->form) ob->form->redraw = 1; }
}

void fl_set_object_color(FL_OBJECT *ob, int col, int pcol)
{
  if (ob->col != col || ob->pcol != pcol) {
    ob->col = col; ob->pcol = pcol;
    if (ob->form) ob->form->redraw = 1;
  }
}

void fl_set_object_label(FL_OBJECT *ob, char *label)
{
  if (strcmp(ob->label, label)) { strcpy(ob->label, label); if (ob->form) ob->form->redraw = 1; }
}

void fl_set_object_lcol(FL_OBJECT *ob, int c)
{
  if (ob->lcol != c) { ob->lcol = c; if (ob->form) ob->form->redraw = 1; }
}

void fl_set_object_bcol(FL_OBJECT *ob, int c)
{
  if (ob->bcol != c) { ob->bcol = c; if (ob->form) ob->form->redraw = 1; }
}

void fl_set_object_tcol(FL_OBJECT *ob, int c)
{
  if (ob->tcol != c) { ob->tcol = c; if (ob->form) ob->form->redraw = 1; }
}

void fl_set_object_align(FL_OBJECT *ob, int align)
{
  if (ob->align != align) { ob->align = align; if (ob->form) ob->form->redraw = 1; }
}

void fl_hide_object(FL_OBJECT *ob)
{
  int n;
  for (n = 0; ob; ob = ob->next) {
    if (ob->class = FL_BEGIN_GROUP) n++;
    else if (ob->class = FL_END_GROUP) { if (n) n--; }
    else if (!ob->hide++ && ob->form) ob->form->redraw = 1;
    if (!n) break;
  }
}

void fl_show_object(FL_OBJECT *ob)
{
  int n;
  for (n = 0; ob; ob = ob->next) {
    if (ob->class = FL_BEGIN_GROUP) n++;
    else if (ob->class = FL_END_GROUP) { if (n) n--; }
    else if (ob->hide) { if (!--ob->hide && ob->form) ob->form->redraw = 1; }
    if (!n) break;
  }
}

void fl_deactivate_object(FL_OBJECT *ob)
{
  int n;
  for (n = 0; ob; ob = ob->next) {
    if (ob->class = FL_BEGIN_GROUP) n++;
    else if (ob->class = FL_END_GROUP) { if (n) n--; }
    else if (!ob->inactive++ && ob->form) ob->form->redraw = 1;
    if (!n) break;
  }
}

void fl_activate_object(FL_OBJECT *ob)
{
  int n;
  for (n = 0; ob; ob = ob->next) {
    if (ob->class = FL_BEGIN_GROUP) n++;
    else if (ob->class = FL_END_GROUP) { if (n) n--; }
    else if (ob->inactive) { if (!--ob->inactive && ob->form) ob->form->redraw = 1; }
    if (!n) break;
  }
}

void fl_set_call_back(FL_OBJECT *ob, void (*call)(), int arg)
{
  ob->call = call; ob->arg = arg;
}

void fl_set_focus_object(FL_FORM *form, FL_OBJECT *ob)
{
  if (form->focus && form->focus != ob && form->focus->class == FL_INPUT) {
    form->redraw = 1;
    if (form->focus->call) form->focus->call(form->focus, form->focus->arg); else if (form->call) form->call(form->focus);
  }
  form->focus = ob;
}

// forms
FL_FORM *fl_bgn_form(int boxtype, int w, int h)
{
  fl_current_form = (FL_FORM *) memset(malloc(sizeof(FL_FORM)), 0, sizeof(FL_FORM));
  fl_current_form->w = w;
  fl_current_form->h = h;
  fl_current_form->winid = -1;
  fl_current_form->col = boxtype ? fl_form_color : FL_BLACK;
  fl_current_form->redraw = 1;
  fl_current_form->hide = 1;
  fl_current_form->next = fl_forms; fl_forms = fl_current_form;
  return fl_current_form;
}

void fl_end_form(void)
{
  fl_current_form = 0;
}

void fl_addto_form(FL_FORM *form)
{
  fl_current_form = form;
}

FL_OBJECT *fl_bgn_group(void)
{
  FL_OBJECT *ob = (FL_OBJECT *) memset(malloc(sizeof(FL_OBJECT)), 0, sizeof(FL_OBJECT));
  ob->class = FL_BEGIN_GROUP;
  fl_add_object(fl_current_form, ob);
  return ob;
}

FL_OBJECT *fl_end_group(void)
{
  FL_OBJECT *ob = (FL_OBJECT *) memset(malloc(sizeof(FL_OBJECT)), 0, sizeof(FL_OBJECT));
  ob->class = FL_END_GROUP;
  fl_add_object(fl_current_form, ob);
  return ob;
}

void fl_hide_form(FL_FORM *form)
{
  FL_FORM *f;
  if (!form->hide) {
    if (fl_mouseob && fl_mouseob->form == form) fl_mouseob = 0;
    if (fl_pushob && fl_pushob->form == form) { if (fl_pushob->handle) fl_pushob->handle(fl_pushob, LUP, 0, 0); fl_pushob = 0; }
    winclose(form->winid);
    form->winid = -1;
    form->hide = 1;
    if (form->modal) { for (f = fl_forms; f; f = f->next) if (!f->hide && f->inactive) f->inactive--; }
    form->redraw = 1;
  }
}

int fl_show_form(FL_FORM *form, int place, int border, char *label) // XXX place, border
{
  FL_FORM *f;
  if (form->hide) {
    if (form->modal) { for (f = fl_forms; f; f = f->next) if (!f->hide) f->inactive++; }
    form->hide = 0;
    prefsize(form->w, form->h);
    form->winid = winopen(label);

    glClearDepth(1.0);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (!default_texture)
      load_fonts();
    else
      glBindTexture(GL_TEXTURE_2D, default_texture);

    form->redraw = 1;
  }
  return form->winid;
}

void fl_deactivate_form(FL_FORM *form)
{
  form->inactive++;
}

void fl_activate_form(FL_FORM *form)
{
  if (form->inactive) form->inactive--;
}

void fl_deactivate_all_forms(void)
{
  FL_FORM *form;
  for (form = fl_forms; form; form = form->next) form->inactive++;
}

void fl_activate_all_forms(void)
{
  FL_FORM *form;
  for (form = fl_forms; form; form = form->next) if (form->inactive) form->inactive--;
}

void fl_set_form_call_back(FL_FORM *form, void (*call)())
{
  form->call = call;
}

void fl_set_form_title(FL_FORM *form, char *title)
{
  if (!form->hide) {
    winset(form->winid);
    wintitle(title);
  }
}

// box object
int handle_box(FL_OBJECT *ob, int event)
{
  if (event == REDRAW) {
    fl_draw_box(ob->boxtype, ob->x, ob->y, ob->w, ob->h, ob->col, ob->bcol);
    if (ob->lcol) { fl_color(ob->lcol); fl_draw_label(ob->align, ob->x, ob->y, ob->w, ob->h, ob->label); }
  }
  return 0;
}

FL_OBJECT *fl_create_box(int type, int x, int y, int w, int h, char *label)
{
  FL_OBJECT *ob;
  ob = fl_make_object(FL_BOX, type, x, y, w, h, label, handle_box);
  ob->boxtype = type;
  ob->inactive = 1;
  ob->align = FL_ALIGN_CENTER;
  return ob;
}

FL_OBJECT *fl_add_box(int type, int x, int y, int w, int h, char *label)
{
  FL_OBJECT *ob;
  fl_add_object(fl_current_form, ob = fl_create_box(type, x, y, w, h, label));
  return ob;
}

// text object
FL_OBJECT *fl_add_text(int type, int x, int y, int w, int h, char *label)
{
  FL_OBJECT *ob;
  ob = fl_create_box(FL_NO_BOX, x, y, w, h, label);
  ob->align = FL_ALIGN_LEFT;
  fl_add_object(fl_current_form, ob);
  return ob;
}

// button object
void fl_set_button(FL_OBJECT *ob, int pushed)
{
  FL_OBJECT *b; int redraw = 0;
  if (ob->type == FL_RADIO_BUTTON) {
    for (b = ob->prev; b && b->type == FL_RADIO_BUTTON; b = b->prev) { if (b->val) { b->val = 0; redraw = 1; } } // XXX removing closing } crashes compiler!
    for (b = ob->next; b && b->type == FL_RADIO_BUTTON; b = b->next) { if (b->val) { b->val = 0; redraw = 1; } }
  }
  if (ob->val != pushed) { ob->val = pushed; redraw = 1; }
  if (ob->form) ob->form->redraw |= redraw;
}

int fl_get_button(FL_OBJECT *ob)
{
  return ob->val;
}

int handle_button(FL_OBJECT *ob, int event)
{
  static int oldval; int newval;
  switch (event) {
  case REDRAW:
    if (ob->val) fl_draw_box(ob->boxtype == FL_UP_BOX  ? FL_DOWN_BOX   : ob->boxtype, ob->x, ob->y, ob->w, ob->h, ob->pcol, ob->bcol);
    else         fl_draw_box(ob->boxtype == FL_TAB_BOX ? FL_BORDER_BOX : ob->boxtype, ob->x, ob->y, ob->w, ob->h, ob->col,  ob->bcol);
    if (ob->lcol) {
      fl_color(ob->lcol); fl_draw_label(ob->align, ob->x, ob->y, ob->w, ob->h, ob->label);
      if ((ob->type == FL_RADIO_BUTTON || ob->type == FL_TOGGLE_BUTTON || ob->type == FL_TOUCH_BUTTON) && 
          ob->boxtype == FL_BORDER_BOX && ob->val && !(ob->align == FL_ALIGN_CENTER && ob->label[0]))
        fl_draw_label(FL_ALIGN_CENTER, ob->x, ob->y, ob->w, ob->h, "X");
    }
    break;

  case LDOWN:
    if (ob->type == FL_RADIO_BUTTON) {
      if (!ob->val) { fl_set_button(ob, 1); return 1; }
      return 0;
    }
    ob->form->redraw = 1;
    ob->val = 1 - (oldval = ob->val);
    return (ob->type == FL_TOUCH_BUTTON);

  case LUP:
    if (ob->type == FL_RADIO_BUTTON) break;
    ob->form->redraw = 1;
    if (ob->type == FL_TOUCH_BUTTON) return (ob->val != oldval);
    else if (!ob->val) return 0;
    ob->val = 0;
    return (ob->type != FL_TOUCH_BUTTON);

  case LMOUSE:
    if (ob->type == FL_RADIO_BUTTON) break;
    newval = (ob == fl_mouseob) ? (1 - oldval) : oldval;
    if (ob->val != newval) { ob->val = newval; ob->form->redraw = 1; }
  }
  return 0;
}

FL_OBJECT *fl_create_button(int type, int x, int y, int w, int h, char *label)
{
  FL_OBJECT *ob;
  ob = fl_make_object(FL_BUTTON, type, x, y, w, h, label, handle_button);
  ob->boxtype = (type == FL_RADIO_BUTTON) ? FL_BORDER_BOX : FL_UP_BOX;
  return ob;
}

FL_OBJECT *fl_add_button(int type, int x, int y, int w, int h, char *label)
{
  FL_OBJECT *ob;
  fl_add_object(fl_current_form, ob = fl_create_button(type, x, y, w, h, label));
  return ob;
}

// form handling
void fl_handle_forms(int e, int w, int x, int y) // XXX figure out how to handle HANGUP, WINSHUT and SHAPE
{
  FL_FORM *form; FL_OBJECT *ob;
  for (form = fl_forms; form; form = form->next) if (form->winid == w) goto found;
  return;
found:
  if (e == REDRAW) { form->redraw = 1; return; }
  if (form->inactive) return;
  if (e == KEY) {
    ob = form->focus;
  } else {
    for (ob = form->last; ob; ob = ob->prev)
      if (!ob->hide && !ob->inactive && x >= ob->x && x < ob->x+ob->w && y >= ob->y && y < ob->y+ob->h) break;
    fl_mouseob = ob;
    switch (e) {
    case LDOWN:
      if (form->focus && form->focus != ob && form->focus->class == FL_INPUT) {
        form->redraw = 1;
        if (form->focus->call) form->focus->call(form->focus, form->focus->arg); else if (form->call) form->call(form->focus);
      }
      form->focus = fl_pushob = ob;
      break;
    case LUP: ob = fl_pushob; fl_pushob = 0; break;
    case LMOUSE: ob = fl_pushob; break;
    default: return; // XXX  passed to the handle routine instead?
    }
  }
  if (ob && ob->handle && ob->handle(ob, e, x, y)) if (ob->call) ob->call(ob, ob->arg); else if (ob->form->call) ob->form->call(ob);
}

int fl_redraw_form(FL_FORM *form)
{
  uint c; FL_OBJECT *ob;
  if (!form->hide) {
    winset(form->winid);
    glViewport(0, 0, form->w, form->h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, form->w, 0.0, form->h, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
      
    c = form->col;
    glClearColor((c>>24)/255.0, ((c>>16)&255)/255.0, ((c>>8)&255)/255.0, (c&255)/255.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (ob = form->first; ob; ob = ob->next) { if (!ob->hide && ob->handle) ob->handle(ob, REDRAW, 0, 0); }
    form->redraw = 0;
    swapbuffers();
    return 1;
  }
  return 0;
}

int fl_draw_forms(void)
{
  FL_FORM *form; int redraw = 0;
  for (form = fl_forms; form; form = form->next) if (form->redraw) redraw |= fl_redraw_form(form);
  return redraw;
}

int fl_initialize() //int argc, char *argv[], char *label, int x, int y)
{
  return ginit();
}
