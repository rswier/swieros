// calc.c -- calculator 
//
// Usage:  calc
//
// Description:
//   calc is a graphical calculator.

// Based on xforms library demo code by Mark Overmars.

#include <u.h>
#include <libc.h>
#include <libm.h>
#include <net.h>
#include <gl.h>
#include <font.h>
#include <forms.h>

double PI = 3.14159265358979323846;

enum {
  PLUS, MIN, MUL_, DIV_, POW, INVPOW, // operators
  EXP = -3, NORM, DOT,     // display modes
  BRAKS = 10,
};

int priority[6] = {1, 1, 2, 2, 3, 3}; // priorities of each operator
double value[4*(BRAKS+1)]; // values on the stack
int oper[3*(BRAKS+1)];     // operators between them
int top;                   // top of the stack
int startbrkt[BRAKS+1];    // positions of the left brackets
int currentbrkt;           // bracketing we are in
double mem;                // memory value
int ready;                 // whether last number is ready. if "ready" is set, then typing another number overwrites the current number.
int dot;                   // whether the dot has been typed
double diver;              // divider when behind the dot
int behind;                // number of digits behind dot
int inv;                   // whether inverse key is depressed
int emode;                 // whether we are entering the exponent
int exponent;              // exponent value whilst entering exponent
double mantissa;           // mantissa value whilst entering exponent
int base = 10;             // base we are working in (2,8,10 or 16)
int drgmode;               // deg, rad or grad mode

FL_OBJECT *zerobut,   *dotbut,    *pmbut,     *epibut,    *isbut;
FL_OBJECT *onebut,    *twobut,    *threebut,  *plusbut,   *minbut; 
FL_OBJECT *fourbut,   *fivebut,   *sixbut,    *mulbut,    *divbut;
FL_OBJECT *sevenbut,  *eightbut,  *ninebut,   *clearbut,  *allcbut;
FL_OBJECT *memaddbut, *memmulbut, *memcbut,   *memstobut, *memrclbut;
FL_OBJECT *lbrktbut,  *rbrktbut,  *exchbut,   *recipbut,  *factbut;
FL_OBJECT *logbut,    *lnbut,     *intbut,    *dtorbut,   *drgbut;
FL_OBJECT *sqrtbut,   *powbut,    *sinbut,    *cosbut,    *tanbut;
FL_OBJECT *invbut,    *exitbut;
FL_OBJECT *base2but,  *base8but,  *base10but, *base16but;
FL_OBJECT *drgdisp,   *brktdisp,  *memdisp,   *display;

double mytrunc(double x)
{
  return x < 0.0 ? -floor(-x) : floor(x);
}

double copysign(double x, double y)
{
  return y >= 0.0 ? x : -x;
}

setlabels()
{
  fl_set_object_label(sqrtbut, base > 10 ? "a" : (inv ? "x^2" : "sqrt"));
  fl_set_object_label(powbut,  base > 10 ? "b" : (inv ? "x^1/y" : "x^y"));
  fl_set_object_label(sinbut,  base > 10 ? "c" : (inv ? "arsin" : "sin"));
  fl_set_object_label(cosbut,  base > 10 ? "d" : (inv ? "arcos" : "cos"));
  fl_set_object_label(tanbut,  base > 10 ? "e" : (inv ? "artan" : "tan"));
  fl_set_object_label(logbut,  base > 10 ? "f" : (inv ? "10^x" : "log"));

  fl_set_object_label(lnbut,     inv ? "e^x" : "ln");
  fl_set_object_label(intbut,    inv ? "frac" : "int");
  fl_set_object_label(dtorbut,   inv ? "r->d" : "d->r");
  fl_set_object_label(memaddbut, inv ? "M-" : "M+");
  fl_set_object_label(memmulbut, inv ? "M/" : "M*");
  fl_set_object_label(memcbut,   inv ? "Mex" : "MC");
}

cvttobase(double num, int base, int behind, char *str)
{
  double sign, div;
  int place, digit, i;
  char digstr[2];

  sign = copysign(1.0, num);
  num *= sign;
  if (sign == -1.0)
    sprintf(str, "-");
  else
    sprintf(str, "");

  if (num == 0.0) {
    sprintf(str, "0");
    if (behind > 0) {
      strcat(str, ".");
      for(i = 0; i < behind; i++) strcat(str, "0");
    }
    return;
  }

  place = (int)(log(num)/log(base));
  if (place < 0) place = 0;

  do {
    div = pow(base, place);
    digit = (int)(num/div);
    num -= digit*div;
    if (place == -1) strcat(str, ".");
    place--;
    sprintf(digstr, "%x", digit);
    strcat(str, digstr);
    if (strlen(str) > 18) {
      sprintf(str, "can't display");
      return;
    }
  } while (place >= 0 || (place >= -9 && num != 0.0));

  if (place == -1 && (behind == DOT || behind > 0)) strcat(str, ".");
  while (behind > 0 && behind >= -place) {
    strcat(str, "0");
    place--;
  }
}

set_display(double val, int behind)
{
  int i;
  char dispstr[40], expstr[10], str2[10];

  // number or operator handled to get here so reset inv stuff
  if (inv) {
    inv = 0;
    fl_set_button(invbut, 0);
    setlabels();
  }

  if (behind >= 0) { // format with appropriate number of decimal places
    if (base == 10) {
      emode = 0;
      sprintf(dispstr,"%.*f",behind,val);
    }
    else // non base 10 display
      cvttobase(val, base, behind, dispstr);
  }
  else if (behind == DOT) { // display the . at the right
    if (base == 10) {
      emode = 0;
      sprintf(dispstr,"%.1f",val);
      dispstr[strlen(dispstr)-1] = 0;
    }
    else
      cvttobase(val, base, behind, dispstr);
  }
  else if (behind == NORM) { // normal display
    if (base == 10) {
      emode = 0;
//      sprintf(dispstr,"%.9g",val);  XXX
      sprintf(dispstr,"%.9f",val);
    }
    else // non base 10 display
      cvttobase(val, base, behind, dispstr);
  }
  else { // exponent entering display
    sprintf(dispstr,"%.8f",val);
    for (i = strlen(dispstr); dispstr[i-1] == '0'; i--);
    dispstr[i] = 0;
    strcat(dispstr, "e");
    sprintf(expstr,"%d",exponent);
    strcat(dispstr, expstr);
  }
  strcat(dispstr," ");
  dispstr[17] = 0;
  fl_set_object_label(display, dispstr);
}

set_memdisp()
{
  fl_set_object_label(memdisp, mem ? "M" : "");
}

set_brktdisp()
{
  char dispstr[40];
  if (currentbrkt > 0) {
    sprintf(dispstr, "%d [ max %d", currentbrkt, BRAKS);
    fl_set_object_label(brktdisp, dispstr);
  }
  else
    fl_set_object_label(brktdisp, "");
}

init_value(int lev) // initializes adding a value to the stack
{
  top = lev;
  value[top] = 0.0;
  ready = 0;
  emode = 0;
  dot = 0;
  diver = 1.0;
  behind = 0;
  if (inv) {
    inv = 0;
    fl_set_button(invbut, 0);
    setlabels();
  }
}

handle_numb(int numb) // handles typing a number
{
  int first;
  double sign;

  if (ready) init_value(top);

  if (emode) {
    sign = copysign(1.0, exponent);
    if (abs(exponent)*10 + numb > 999) { // cycle if exponent has > 3 digits
      first = floor(abs(exponent)/100.0);
      exponent = abs(exponent) - 100*first;
      exponent *= (int)sign;
    }
    exponent = exponent*10 + (int) (sign*numb);
    value[top] = mantissa*pow(10.0, exponent);
    set_display(mantissa, EXP);
  }
  else if (numb < base) { // both decimal and non decimal number entry
    sign = copysign(1.0, value[top]);
    if (dot && behind < 9) {
      behind++;
      diver = diver/base;
      value[top] += sign*diver*numb;
    }
//    else if ((! dot) && (value[top] < 1.0e10)) XXX
    else if ((! dot) && (value[top] < 10000000.0))
      value[top] = base*value[top] + sign*numb;

    set_display(value[top],behind);
  }
}

calc(int i) // calculates an entry in the stack
{
  switch (oper[i]) {
    case PLUS:   value[i] += value[i+1]; break;
    case MIN:    value[i] -= value[i+1]; break;
    case MUL_:   value[i] *= value[i+1]; break;
    case DIV_:   value[i] /= value[i+1]; break;
    case POW:    value[i] = pow(value[i], value[i+1]); break;
    case INVPOW: value[i] = pow(value[i], 1.0 / value[i+1]); break;
  }
}

changebase(int newbase)
{
  int oldbase = base;
  base = newbase;
  set_display(value[top], NORM);
  ready = 1;
  if (oldbase == 16 || base == 16) setlabels();
}

exch() // handles an exchange of the last 2 values
{
  double temp;
  // check if we have 2 values to exchange
  if (top > startbrkt[currentbrkt]) {
    temp = value[top];
    value[top] = value[top-1];
    value[top-1] = temp;
    set_display(value[top], NORM);
    ready = 1;
  }
}

memexch() // handles an exchange between memory and last val
{
  double temp = mem;
  mem = value[top];
  value[top] = temp;
  set_display(value[top], NORM);
  ready = 1;
  set_memdisp();
}

int signgam = 1; // XXX
double gamma(double x)
{
  return x;    // XXX
}

factorial()
{
  int signgam;
  double lg, alpha;
  // uses gamma functions to get result for non-integer values
  alpha = value[top] + 1.0;
  if (floor(alpha) == alpha && alpha <= 0.0) {
    init_value(0);
    fl_set_object_label(display, "Error: -ve integer ");
  }
  else {
    lg = gamma(alpha);
    value[top] = signgam*exp(lg);
    set_display(value[top], NORM);
    ready = 1;
  }
}

epi()
{
  if (value[top] == 0.0 || ready) {
    value[top] = PI;
    set_display(value[top], NORM);
    ready = 1;
  }
  else if (!emode && base == 10) {
    emode = 1;
    exponent = 0;
    mantissa = value[top];
    set_display(mantissa,EXP);
  }
}

double todrg(double angle)
{
  if (drgmode == 0)
    return PI*angle/180.0;
  else if (drgmode == 2)
    return PI*angle/100.0;
  else
    return angle;
}

double fromdrg(double angle)
{
  if (drgmode == 0)
    return 180.0*angle/PI;
  else if (drgmode == 2)
    return 100.0*angle/PI;
  else
    return angle;
}

lbrkt()
{
  if (currentbrkt < BRAKS) {
    currentbrkt++;
    startbrkt[currentbrkt] = top;
    ready = 1;
    set_brktdisp();
  }
}

rbrkt()
{
  int i;
  if (currentbrkt > 0) {
    for (i = top; i > startbrkt[currentbrkt]; i--) calc(i-1);
    top = startbrkt[currentbrkt];
    currentbrkt--;
    ready = 1;
  }
  set_display(value[top], NORM);
  set_brktdisp();
}

handle_oper(int op)
{
  int prevop, i, finished;
  finished = 0;
  do {
    if (top == startbrkt[currentbrkt]) finished = 1; // is 1st operator
    if (! finished) { // compare priority of previous operators with current op
      prevop = oper[top-1];
      if (priority[prevop] < priority[op])
        finished = 1;
      else { // last op can be calculated
        top--;
        calc(top);
      }
    }
  } while (! finished);
  oper[top] = op;
  init_value(top+1);
  set_display(value[top-1], NORM);
}

is()
{
  int i;
  while (currentbrkt > 0) rbrkt();
  for (i = top; i > 0; i--) calc(i-1);
  top = 0;
  ready = 1;
  set_display(value[top], NORM);
}

void handle_but(FL_OBJECT *but)
{
  if      (but == onebut)   handle_numb(1);
  else if (but == twobut)   handle_numb(2);
  else if (but == threebut) handle_numb(3);
  else if (but == fourbut)  handle_numb(4);
  else if (but == fivebut)  handle_numb(5);
  else if (but == sixbut)   handle_numb(6);
  else if (but == sevenbut) handle_numb(7);
  else if (but == eightbut) handle_numb(8);
  else if (but == ninebut)  handle_numb(9);
  else if (but == zerobut)  handle_numb(0);
  else if (but == dotbut) {
    if (ready) init_value(top);
    if (!dot) { dot = 1; set_display(value[top],DOT); }
  }
  else if (but == plusbut)  handle_oper(PLUS);
  else if (but == minbut)   handle_oper(MIN);
  else if (but == mulbut)   handle_oper(MUL_);
  else if (but == divbut)   handle_oper(DIV_);
  else if (but == powbut) {
    if (base > 10) handle_numb(11); else handle_oper(inv ? INVPOW : POW);
  }
  else if (but == isbut)    is();
  else if (but == lbrktbut) lbrkt();
  else if (but == rbrktbut) rbrkt();

  else if (but == epibut)   epi();
  else if (but == factbut)  factorial();
  else if (but == exchbut)  exch();
  else if (but == pmbut) {
    if (! emode) { value[top] = -value[top]; set_display(value[top], NORM); }
    else { exponent = -exponent; value[top] = mantissa*pow(10.0, exponent); set_display(mantissa,EXP); }
  }
  else if (but == recipbut) {
    value[top] = 1.0/value[top];
    set_display(value[top], NORM);
    ready = 1;
  }
  else if (but == clearbut) { init_value(top); set_display(0.0, NORM); }
  else if (but == allcbut)  {
    init_value(0);
    set_display(0.0, NORM);
    currentbrkt = 0;
    fl_set_object_label(brktdisp, "");
  }
  else if (but == memcbut) {
    if (!inv) {
      mem = 0.0;
      set_display(value[top], NORM);
      ready = 1;
      set_memdisp();
    }
    else memexch();
  }
  else if (but == memrclbut) {
    value[top] = mem;
    set_display(value[top], NORM);
    ready = 1;
  }
  else if (but == memstobut) {
    mem = value[top];
    set_display(value[top], NORM);
    ready = 1;
    set_memdisp();
  }
  else if (but == memaddbut) {
    if (!inv) mem += value[top]; else mem -= value[top];
    set_display(value[top], NORM);
    ready = 1;
    set_memdisp();
  }
  else if (but == memmulbut) {
    if (!inv) mem *= value[top]; else mem /= value[top];
    set_display(value[top], NORM);
    ready = 1;
    set_memdisp();
  }
  else if (but == drgbut) {
    drgmode = (drgmode + 1) % 3;
    fl_set_object_label(drgdisp, drgmode == 0 ? "DEG" : (drgmode == 1 ? "RAD" : "GRAD"));
  }
  else if (but == invbut) { inv ^= 1; setlabels(); }
  else if (but == base2but)  changebase(2);
  else if (but == base8but)  changebase(8);
  else if (but == base10but) changebase(10);
  else if (but == base16but) changebase(16);

  else if (but == sqrtbut) {
    if (base > 10) handle_numb(10);
    else if (!inv) { value[top] = sqrt(value[top]); set_display(value[top], NORM); ready = 1; }
    else { value[top] = pow(value[top], 2.0); set_display(value[top], NORM); ready = 1; }
  }
  else if (but == sinbut) {
    if (base > 10) handle_numb(12);
    else if (!inv) { value[top] = sin(todrg(value[top])); set_display(value[top], NORM); ready = 1; }
    else { value[top] = fromdrg(asin(value[top])); set_display(value[top], NORM); ready = 1; }
  }
  else if (but == cosbut) {
    if (base > 10) handle_numb(13);
    else if (!inv) { value[top] = cos(todrg(value[top])); set_display(value[top], NORM); ready = 1; }
    else { value[top] = fromdrg(acos(value[top])); set_display(value[top], NORM); ready = 1; }
  }
  else if (but == tanbut) {
    if (base > 10) handle_numb(14);
    else if (!inv) { value[top] = tan(todrg(value[top])); set_display(value[top], NORM); ready = 1; }
    else { value[top] = fromdrg(atan(value[top])); set_display(value[top], NORM); ready = 1; }
  }
  else if (but == logbut) {
    if (base > 10) handle_numb(15);
    else if (!inv) { value[top] = log10(value[top]); set_display(value[top], NORM); ready = 1; }
    else { value[top] = pow(10.0, value[top]); set_display(value[top], NORM); ready = 1; }
  }
  else if (but == lnbut) {
    if (!inv) { value[top] = log(value[top]); set_display(value[top], NORM); ready = 1; }
    else { value[top] = exp(value[top]); set_display(value[top], NORM); ready = 1; }
  }
  else if (but == intbut) {
    if (!inv) { value[top] = mytrunc(value[top]); set_display(value[top], NORM); ready = 1; }
    else { value[top] = value[top] - mytrunc(value[top]); set_display(value[top], NORM); ready = 1; }
  }

  else if (but == dtorbut) {
    if (!inv) { value[top] = PI*value[top]/180.0; set_display(value[top], NORM); ready = 1; }
    else { value[top] = 180.0*value[top]/PI; set_display(value[top], NORM); ready = 1; }
  }
  else if (but == exitbut) {
    exit(0);
  }
}

FL_FORM *make_calc()
{
  FL_FORM *form;
  
  form = fl_bgn_form(FL_FLAT_BOX, 280, 440);

  zerobut   = fl_add_button(FL_NORMAL_BUTTON,  20, 20, 40, 30, "0");
  dotbut    = fl_add_button(FL_NORMAL_BUTTON,  70, 20, 40, 30, ".");
  pmbut     = fl_add_button(FL_NORMAL_BUTTON, 120, 20, 40, 30, "+/-");
  epibut    = fl_add_button(FL_NORMAL_BUTTON, 170, 20, 40, 30, "E/pi");
  isbut     = fl_add_button(FL_NORMAL_BUTTON, 220, 20, 40, 30, "=");

  onebut    = fl_add_button(FL_NORMAL_BUTTON,  20, 60, 40, 30, "1");
  twobut    = fl_add_button(FL_NORMAL_BUTTON,  70, 60, 40, 30, "2");
  threebut  = fl_add_button(FL_NORMAL_BUTTON, 120, 60, 40, 30, "3");
  plusbut   = fl_add_button(FL_NORMAL_BUTTON, 170, 60, 40, 30, "+");
  minbut    = fl_add_button(FL_NORMAL_BUTTON, 220, 60, 40, 30, "-");

  fourbut   = fl_add_button(FL_NORMAL_BUTTON,  20, 100, 40, 30, "4");
  fivebut   = fl_add_button(FL_NORMAL_BUTTON,  70, 100, 40, 30, "5");
  sixbut    = fl_add_button(FL_NORMAL_BUTTON, 120, 100, 40, 30, "6");
  mulbut    = fl_add_button(FL_NORMAL_BUTTON, 170, 100, 40, 30, "*");
  divbut    = fl_add_button(FL_NORMAL_BUTTON, 220, 100, 40, 30, "/");
  
  sevenbut  = fl_add_button(FL_NORMAL_BUTTON,  20, 140, 40, 30, "7");
  eightbut  = fl_add_button(FL_NORMAL_BUTTON,  70, 140, 40, 30, "8");
  ninebut   = fl_add_button(FL_NORMAL_BUTTON, 120, 140, 40, 30, "9");
  clearbut  = fl_add_button(FL_NORMAL_BUTTON, 170, 140, 40, 30, "C");
  allcbut   = fl_add_button(FL_NORMAL_BUTTON, 220, 140, 40, 30, "AC");

  memaddbut = fl_add_button(FL_NORMAL_BUTTON,  20, 180, 40, 30, "M+");
  memmulbut = fl_add_button(FL_NORMAL_BUTTON,  70, 180, 40, 30, "M*");
  memcbut   = fl_add_button(FL_NORMAL_BUTTON, 120, 180, 40, 30, "MC");
  memstobut = fl_add_button(FL_NORMAL_BUTTON, 170, 180, 40, 30, "Mst");
  memrclbut = fl_add_button(FL_NORMAL_BUTTON, 220, 180, 40, 30, "Mrc");

  lbrktbut  = fl_add_button(FL_NORMAL_BUTTON,  20, 220, 40, 30, "[");
  rbrktbut  = fl_add_button(FL_NORMAL_BUTTON,  70, 220, 40, 30, "]");
  exchbut   = fl_add_button(FL_NORMAL_BUTTON, 120, 220, 40, 30, "exch");
  recipbut  = fl_add_button(FL_NORMAL_BUTTON, 170, 220, 40, 30, "1/x");
  factbut   = fl_add_button(FL_NORMAL_BUTTON, 220, 220, 40, 30, "x!");

  logbut    = fl_add_button(FL_NORMAL_BUTTON,  20, 260, 40, 30, "log");
  lnbut     = fl_add_button(FL_NORMAL_BUTTON,  70, 260, 40, 30, "ln");
  intbut    = fl_add_button(FL_NORMAL_BUTTON, 120, 260, 40, 30, "int");
  dtorbut   = fl_add_button(FL_NORMAL_BUTTON, 170, 260, 40, 30, "d->r");
  drgbut    = fl_add_button(FL_NORMAL_BUTTON, 220, 260, 40, 30, "d-r-g");

  sqrtbut   = fl_add_button(FL_NORMAL_BUTTON,  20, 300, 40, 30, "sqrt");
  powbut    = fl_add_button(FL_NORMAL_BUTTON,  70, 300, 40, 30, "x^y");
  sinbut    = fl_add_button(FL_NORMAL_BUTTON, 120, 300, 40, 30, "sin");
  cosbut    = fl_add_button(FL_NORMAL_BUTTON, 170, 300, 40, 30, "cos");
  tanbut    = fl_add_button(FL_NORMAL_BUTTON, 220, 300, 40, 30, "tan");

  invbut    = fl_add_button(FL_TOGGLE_BUTTON, 20, 340, 40, 30, "inv");
  exitbut   = fl_add_button(FL_NORMAL_BUTTON,  220, 340, 40, 30, "Exit");

  base2but  = fl_add_button(FL_RADIO_BUTTON,  80, 340, 24, 19, "2");
  base8but  = fl_add_button(FL_RADIO_BUTTON, 110, 340, 24, 19, "8");
  base10but = fl_add_button(FL_RADIO_BUTTON, 140, 340, 23, 19, "10"); fl_set_button(base10but, 1);
  base16but = fl_add_button(FL_RADIO_BUTTON, 170, 340, 24, 19, "16");

  drgdisp   = fl_add_box(FL_FLAT_BOX,  70, 360, 30, 20, "GRAD");
  brktdisp  = fl_add_box(FL_FLAT_BOX, 120, 360, 60, 20, "10 [ max 10");
  memdisp   = fl_add_box(FL_FLAT_BOX, 190, 360, 20, 20, "M");

  display   = fl_add_box(FL_DOWN_BOX, 20, 380, 240, 40, "0 "); fl_set_object_align(display, FL_ALIGN_RIGHT);
  
  fl_end_form();
  
  return form;
}

calc_init()
{
  init_value(0);
  drgmode = 1;
  base = 10;
  currentbrkt = 0;
  startbrkt[0] = 0;
  set_memdisp();
  set_brktdisp();
}

int main(int argc, char *argv[])
{
  int e, w, x, y; FL_FORM *form;
  fl_initialize();
  form = make_calc();
  fl_set_form_call_back(form, handle_but);
  fl_show_form(form, 0, 0, "Calculator"); // XXX
  calc_init();
  fl_draw_forms();
  while (e = qread(&w, &x, &y)) {
    if (e == WINSHUT) break;
    if (e == HANGUP) { dprintf(2,"hangup\n"); break; }
    fl_handle_forms(e, w, x, y);
    fl_draw_forms();
  }
  return 0;
}
