/** \file cryxtels.cpp
 * Crystal Pixels - main source file
 * File created in 12th of September of 1994.
 * Initial version: 1.00
 *
 * Modern version of Crystal Pixels was started in August of 2013.
 *
 * This software is licensed under the GNU GPL v3.

    (c) 1994-95-96-97 Alessandro Ghignola and Fottifoh,
                      both being the same person.

        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 2 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.


        You should have received a copy of the GNU General Public License
        along with the program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <iostream>
#include <random>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <cmath>
#include <cstring>
#include <errno.h>
#include <cassert>
#include "global.h"
#include "fast3d.h"
#include "text3d.h"
#include "scanline.h"
#include "input.h"
#include "sdl_exception.h"

#include <SDL.h>
#include "conf.h"

#ifndef far
#define far
#endif

using namespace std;

constexpr int FRAMES_PER_SECOND = 40;
constexpr int TICKS_IN_A_SECOND = 1000;
constexpr int TICKS_PER_FRAME = TICKS_IN_A_SECOND / FRAMES_PER_SECOND;

// dummy function (nullify effect)
inline void play (long) {}

/// initialize some parts of cryxtels
inline void init_start();

/// perform a broad range of needed memory allocations
/// \return false if any allocation fails
inline bool allocation_farm();

/// read the command line arguments and do stuff
void read_args(int argc, char** argv, char& flag, char& sit);

// Aggiorna il buffer dei pixels caricati.
/// Update buffer with loaded pixels
void cerca_e_carica (int typ);

/// build the cosmos
void build_cosm(char& flag);

/// Update
void Aggiornamento ();

/// Update pixels' absd value (distance to observer)
void dists ();

// Assegnazione posizioni a pixels gravitanti.
/// Assign positions to gravitating pixels.
void rot ();

/// Fade out effect of the display.
void fade ();

/// Docking effects.
void dock_effects ();

/// Save state
void Archivia_Situazione (char i);

/// Load state
void Riproduci_Situazione (char i);

/// Just makes the program exit because of something...
void par0 (int el, int pix);

/// redefinition of random(int),
/// which probably became deprecated or existed in an outdated library
int random(int max_num);

/// Terminate some stuff
void alfin (char arc);

/// Take a snapshot into a BMP file.
void snapshot ();

// Auxiliary variables for collyblock function
char onblock; // Osservatore almeno su un blocco di collisione.
char dropping; // Si stanno lasciando cadere oggetti (fase iniziale).
double lastblock_real_x; // Servono alla funzione lascia_cadere.
double lastblock_real_z;
double max_y;

/* Modifica la posizione dell'osservatore se c' eventuale collisione
   con la zona data dal parallelepipedo definito con i parametri,
   relativi all'origine di un Crystal Pixel. */
/// Modify the observer's position based on the eventual collision
/// With the zone given by a box, relative to the origin of a cryxtel.
void collyblock (double cx, double cy, double cz,
                 double hx, double hy, double hz,
                 char blocktype);

/* Calcola le zone di collisione per il pixel vicino e per gli oggetti su
   di esso. Tenta di prelevare l'oggetto pi vicino. */
/// Calculates the collision zone by neighbour pixel and its objects.
/// It tries to get the nearest object.
void CollyZone (int nopix, char objectblocks);

// Disegna la forma di un Crystal Pixel o di un Oggetto.
/// Draws a Cryxtel's, or an object's, model
void Pixel (int typ);

/// Draws an object
void Object (int tipo);

// Funzione che toglie un oggetto dal microcosmo e lo dá all'utente.
/// Function that takes an object from the microcosm and gives it to the user.
void preleva_oggetto (int nr_ogg); // not implemented

/// Update objects' data over the pixels
void Oggetti_sul_Pixel (char oblige); // not implemented

// Funzione che lascia al suolo l'oggetto prelevato.
/// Function that leaves the taken object on the ground.
void lascia_cadere ();

/// May be for drawing a console key on The Fly's console.
void console_key (const char *serigraph, double x, char cod, char input, char cond_attu, char cond_prec);

// Traccia una linea relativamente alla posizione della nave.
/// Draw a line relative to the position of the ship
void n (double sx, double sy, double sz, double fx, double fy, double fz);

/// Draw text relative to the position of the ship
void nTxt (const char *testo, double x, double y, double z, double sx, double sy);

/// Draw rectangle relative to the position of the ship
void nrect (double x, double y, double z, double l, double h);

/// Thrust / Walk Forward
void ispd ();

/// Stop Thrust / Walk Back
void dspd ();

/// Dock.
void attracco ();

/// Dock/Undock (calls attraco() in case of dock).
void undock ();

/// Enter/Leave The Fly
void extra_stop_extra ();

/// Look for angles starting from coordinates relative to the observer. (alfa & beta)
// Per cercare angoli partendo da coordinate relative all'osservatore.
void find_alfabeta();

/// Invert direction
void fid_on ();

/// Point towards velocity
void lead_on ();

/// Point towards Sunny
void orig_on ();

/// Generate the color palette
void tinte (char satu);

/// Explode
void scoppia (int nr_ogg, double potenza, int var);

/// close file driver?
void chiudi_filedriver ();

/// Fottifoh object model
extern char fotty[277];

int main(int argc, char** argv)
{
    cout << "Crystal Pixels" << endl
        << " version 3.15 originally made by Alessandro Ghignola in 1997" << endl
        << "     ported to modern systems by Eduardo Pinho in 2013" << endl;

    char flag = 0;
    //char sit = 'S'; // < the original would auto-load the default game save (Alex's save)
    char sit = 0;
    int ptyp = 0;
    try {
        init_start();

        update_ctrlkeys();

        read_args(argc, argv, flag, sit);

        load_pixels_def(ptyp);

        build_cosm(flag);

        cout << "\n" << endl;

        // set the display
        init_video();
        tinte (0);

    } catch (int err) {
        cerr << "Error code: " << err << endl;
        return -1;
    } catch (sdl_exception& e) {
        cerr << "Error #" << e.code << " : " << e.what() << endl;
        return -1;
    }

    // declaration of main variables
    int p; // Contatore generico.
    int o; // Idem.

    char bkecho = 1; // bk-up per l'ecoscandaglio

    char yel = 0;

    int i = -1, bki = -1, bki9, bki0, bki1, bki2, bki3; // Tasto premuto.

    char blink = 0; // flag per lampeggo.
    char dist[20]; // Stringhe usate per conversioni.

    double veloc, dsol=0;

    int alfax;


    //double sc = 0;

    unsigned long sync;

    const int DELTA_U = 65;
    const int DELTA_L = 135;

    // Don't run the intro if a game is loaded by the user.
    bool run_intro = (argc!=2);

    cam_y = -4680; alfa = 90;
    ox = 0; oy = 0; oz = 0;

    sprintf (t, "%s'S MICROCOSM", autore_forme);

    //while (!tasto_premuto()&&!mpul) {
    // ...
    //}
    //while (!tasto_premuto() && !mpul) {
    while (run_intro) {
        //if (!sbp_stat) play (0);
        sync = SDL_GetTicks(); //clock();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
              case SDL_QUIT:
                alfin (1);
                exit(0);
                break;
//            case SDL_WINDOWEVENT:
//              switch (event->window.event) {
//                case SDL_WINDOWEVENT_HIDDEN:
//              }
//              break;
              case SDL_KEYDOWN:
              case SDL_MOUSEBUTTONDOWN:
                run_intro = false; // Get out of here.
                break;
              default: break;
            }
        }
        if (!run_intro) break;

        Txt ("CRYSTAL PIXELS", -77, 0, 100, 3, 4, 270, 0);
        Txt ("WRITTEN BETWEEN 1994 AND 1997", -100, 0, 80, 2, 4, 270, 0);
        Txt ("BY ALESSANDRO GHIGNOLA.", -85, 0, 60, 2, 4, 270, 0);
        Txt ("MODERN VERSION MADE IN 2013", -102, 0, -84, 2, 4, 270, 0);
        Txt (t, (1-(double)strlen(t)) * 6, 0, -60, 3, 4, 270, 0);
        if (beta<360) {
            cam_y += 25;
            beta += 2;
            darken_once();
        }
        else {
            c = (c+1)%360;
            bki0 = beta; beta = c;
            cam_y += 140;
            Object (0);
            cam_y -= 140;
            beta = bki0;
//       asm {
//          pusha
//          les di, dword ptr adapted
            SDL_LockSurface(p_surface);
            auto di = static_cast<unsigned char*>(p_surface->pixels);
            unsigned short i = 0;
//          xor ah, ah
//          int 1ah
            // this is used to get the clock ticks
//          xor dh, dh
            unsigned int dx = (SDL_GetTicks()/TICKS_PER_FRAME) & 0xFFFF; //% WIDTH;
//          add di, dx
            i = dx;
//          mov cx, 16000 }
            unsigned int cx = (WIDTH*HEIGHT) >> 2;
            do {
//  _chic:
//          asm {
//              cmp di, 64004
//              jnb _zero
                if (i >= WIDTH*HEIGHT) {
                    //i -= WIDTH*HEIGHT;
                    goto _zero;
                }
//              cmp di, 20804
//              jb shift
                if (i < DELTA_U*WIDTH+4) goto shift;
//              cmp di, 43204
//              jb _zero }
                if (i < DELTA_L*WIDTH+4) goto _zero;
    shift:
//              asm
//              shr byte ptr es:[di], 1
                di[i] >>= 1;
    _zero:
//              asm {
//              cmp cx, 8000
//              jb subt
                if (cx >= (WIDTH*HEIGHT) >> 3) {
//                  add di, 321
                    i += WIDTH + 1;
//                  jmp norm }
                } else {
// subt:
//                  asm
//                  add di, 319
                    i += WIDTH - 1;
                }
//  norm:       asm {
//              loop _chic
            } while (--cx > 0);
//          les di, dword ptr adapted
            di = static_cast<unsigned char*>(p_surface->pixels);
//          add di, 20800
            //di += WIDTH*(DELTA_U);// fottifoh upper limit
//          mov cx, 22400 }
            //cx = (DELTA_L-DELTA_U)*WIDTH; // fottifoh space
            cx = HEIGHT*WIDTH; // all space
//___chic:
                do {
//              asm  {
//              cmp byte ptr es:[di], 0
//              je ___zero
                if (*di != 0) {
//                  dec byte ptr es:[di] }
                    *di -= 1;
                }
//___zero:asm  {
//              inc di
                di++;
//          loop ___chic
            } while (--cx > 0);
//          popa }
        }
        //mouse_input();
        Render();

        while (sync + TICKS_PER_FRAME > SDL_GetTicks());
        //while (sync==clock());
    }

//    ignesci: //pop_audiofile ();
//    ignentra: //push_audiofile ("ECHO");

    if (flag)
            Riproduci_Situazione (sit);
    else {
            cam_z = -20000;
            fade ();
    }

    long rclick = 0;
    //long stso = 0;
    //long gap = 0;

    // key hold variables
    auto thrust_keyhold = false;
    auto back_keyhold = false;

    if (grab_mouse) {
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }

    // Ciclo principale.
    bool quit_now = false;
	do
	{
        sync = SDL_GetTicks();
        /*
	    if (blink) {
            sync = clock();
        } */
        // Effetti del sistema di virata.
        alfa+=alfad;
        if (alfa<0) alfa+=360;
        if (alfa>=360) alfa-=360;
        beta+=betad;
        if (beta<0) beta+=360;
        if (beta>=360) beta-=360;

        // Movimento manuale.
        spd_x -= (double)spd * 0.01 * tsin[beta] * tcos[alfa];
        spd_z += (double)spd * 0.01 * tcos[beta] * tcos[alfa];
        spd_y += (double)spd * 0.01 * tsin[alfa];

        // Movimento inerziale.
        cam_x += spd_x;
        cam_y += spd_y;
        cam_z += spd_z;

        rot (); /* GRAVITAZIONE (SPOSTAMENTO PIXELS ROTANTI). */

        dock_effects (); // Legame d'attracco.

        /* Calcolo distanze assolute
            dei pixels dall'osservatore. */
        dists ();

        /* if (extra) {
                rx = pixel_xdisloc[pix] + rel_x - cam_x;
                ry = pixel_ydisloc[pix] + rel_y - cam_y;
                rz = pixel_zdisloc[pix] + rel_z - cam_z;
                pixel_absd[pix] = sqrt (rx*rx+ry*ry+rz*rz);
            } */

        if (!trackframe&&!explode_count) {
            pix = 0;
            for (p=0; p<pixels; p++)
                if (pixel_absd[pix]>pixel_absd[p]) {
                    if (carried_pixel>-1&&p==pixels-1) break;
                    pix = p;
                }
        }
        //gap = pixel_absd[pix] / 500 + 5;

        r_x = rel_x;
        r_z = rel_z;

        // Sezione interazione da mouse.
        if (m && !req_end_extra) {

            mpul = 0;
            mouse_input ();

            if (m && grab_mouse) { // Keep the mouse on the screen
                //SDL_WarpMouse(WIDTH_SCALED/2, HEIGHT_SCALED/2);
                mx += mdltx;
                my += mdlty;
            }

            if (!extra) {
                if (trackframe) {
                    my = 0;
                    if (trackframe==23)
                        mx = (360-nav_b) * 5;
                    else {
                        mx -= 2*mdltx;
                        if (mx>1799) mx -= 1800;
                        if (mx<0) mx += 1800;
                        beta = mx / 5;
                    }
                    goto noang;
                }
            }

            mx -= 2*mdltx;
            my -= 2*mdlty;

            while (mx>=1800) mx -= 1800;
            while (mx<0) mx += 1800;
            while (my>=1800) my -= 1800;
            while (my<0) my += 1800;

            beta = mx / 5;
            alfa = my / 5;
        }
noang:
        // Mod keys array update
        update_ctrlkeys();

        // Sezione interazione da tastiera.
        //if (fermo_li) goto no_keys;

        //i = 0;
        SDL_Event sdlevent;
        while (SDL_PollEvent(&sdlevent)) {
            switch (sdlevent.type) {
              case SDL_QUIT:
                quit_now = true;
                break;
              case SDL_KEYUP: {
                i = sdlevent.key.keysym.sym;
                //cout << i << " UP" << endl;
                switch (i) {
                    case keymap_thrust:
                        thrust_keyhold = false;
                        break;
                    case keymap_back:
                        back_keyhold = false;
                        break;
                }
                i = 0;
              case SDL_KEYDOWN: {
                i = sdlevent.key.keysym.sym;
                //cout << i << " DOWN" << endl;
                switch (i) {
/*                  case 0xa1: ob /= 1.05; if (ob<15000) ob = 15000; break;
                    case 0x9d: micro_y -= 0.001; break;
                    case 0x9b: micro_y += 0.001; break;
                    case 0xa0: micro_x += 0.001; break;
                    case 0x98: micro_x -= 0.001; break;
                    case 0x99: ob *= 1.05; break; */
                    case SDLK_RETURN:
                        if (ctrlkeys[0]&16) {
                            toggle_fullscreen();
                            tinte(0);
                        }
                        break;
                    case keymap_up:
                        if ((ctrlkeys[0]&3) || type_mode || fermo_li || m) break;
                        if ((trackframe!=0)&&!extra) break;
                        alfad--;
                        if (alfad<-3) alfad=-3;
                        break;
                    case keymap_down:
                        if ((ctrlkeys[0]&3) || type_mode || fermo_li || m) break;
                        if (trackframe&&!extra) break;
                        alfad++;
                        if (alfad>3) alfad=3;
                        break;
                    case keymap_right:
                        if ((ctrlkeys[0]&3) || type_mode || fermo_li || m) break;
                        if (trackframe&&!extra) break;
                        betad--;
                        if (betad<-3) betad=-3;
                        break;
                    case keymap_left:
                        if ((ctrlkeys[0]&3) || type_mode || fermo_li || m) break;
                        if (trackframe&&!extra) break;
                        betad++;
                        if (betad>3) betad=3;
                        break;
                    case keymap_thrust: //141: // Acc
                        if ((ctrlkeys[0]&3) || type_mode || fermo_li || m) break;
                        thrust_keyhold = true;
                        break;
                    case keymap_back: //145:
                        if ((ctrlkeys[0]&3) || type_mode || fermo_li || m) break;
                        back_keyhold = true;
                        break;
                    case SDLK_F1: // 59:
                        fid_on (); // invert
                        break;
                    case SDLK_F2: // 60:
                        lead_on (); // point at velocity
                        break;
                    case SDLK_F3: // 61:
                        extra_stop_extra (); // leave/enter The Fly
                        break;
                    case SDLK_F4: // 62:
                        undock (); // dock / undock
                        break;
                    case SDLK_F5: // 63:
                        orig_on (); // point towards Sunny
                        break;
                    case SDLK_F6: // 64:
                        snapshot (); // take a snapshot
                        break;
                    case SDLK_F7: // previously scroll lock
                        if (!extra)
                            type_mode = false;
                        else
                            type_mode = !type_mode; // enter text typing mode
                        break;
                    case SDLK_PAGEUP: // pick up or create object
                        if (carry_type==-1) {
                            if (dsol>15000)
                                taking = 1; // pick up an object
                            else {
                                if (_objects<500) // create a new object
                                    carry_type = existant_objecttypes - 1;
                            }
                        }
                        else {
                            if (dsol<15000) {
                                if (carry_type>0) // change the object being created
                                    carry_type--;
                                else // create a new object
                                    carry_type = existant_objecttypes - 1;
                            }
                        }
                        break;
                    case SDLK_PAGEDOWN: // drop the object
                        if (carry_type!=-1) lascia_cadere ();
                        break;
                    case SDLK_e: // Echo toggle
                        if (ctrlkeys[0]&3) break;
                        echo ^= 1;
                        break;
                    case SDLK_x: // The Fly view toggle
                        if (ctrlkeys[0]&3) break;
                        echo ^= 2;
                        break;
                    case SDLK_TAB: /* 9 */ // Toggle climbing
                        if (ctrlkeys[0]&3) break;
                        echo ^= 4;
                        break;
                    case SDLK_i: // Information toggle
                        if (ctrlkeys[0]&3) break;
                        echo^=8;
                        break;
                    case SDLK_m: // Keyboard/Mouse control toggle
                        if (ctrlkeys[0]&3) break;
                        mpul = 0;
                        m = 1 - m;
                        alfad = 0;
                        betad = 0;
                        mx = beta * 5;
                        my = alfa * 5;
                        //SDL_ShowCursor(m?SDL_DISABLE:SDL_ENABLE);
                        break;
                    default:
                        break;
                    }
                    // Text Cursor
                    switch (i) {
                        case SDLK_RIGHT: //77:
                            if (cursore<511) cursore++;
                            break;
                        case SDLK_LEFT: //75:
                            if (cursore>0) cursore--;
                            break;
                        case SDLK_DOWN: //80:
                            if (cursore<480) cursore += 32;
                            break;
                        case SDLK_UP: //72:
                            if (cursore>31) cursore -= 32;
                            break;
                    }

                    // Loading and saving
                    if ((i>=SDLK_a&&i<=SDLK_z) || i == SDLK_ASTERISK) {
                        if (ctrlkeys[0]&1) {
                            Riproduci_Situazione (i);
                        } else if (ctrlkeys[0]&2) {
                            Archivia_Situazione (i);
                        }
                    }
////                }
//                else {
                    /* Text typing something */
                    if (type_mode) {
                        if (i >= SDLK_a && i <= SDLK_z) {
                            i += 'A'-SDLK_a;
                        }
                        if (i>=32 && i<=96) {
                            type_this = i;
                        }
                    }
                    /*
                    if (ctrlkeys[0]&64) { // caps lock
                        if (ctrlkeys[0]&3) {
                            if (i>='a'&&i<='z')
                                i -= 32;
                        }
                        else {
                            if (i>='A'&&i<='Z')
                                i += 32;
                        }
                    }
                    */
                    if (dsol<15000&&(i>=SDLK_0&&i<=SDLK_9)) {
                        if (carry_type>-1)
                            carry_type = ((int)i-SDLK_0) * existant_objecttypes / 10;
                        if ((ctrlkeys[0]&64)&&carried_pixel>-1) { // caps lock && is carrying pixel
                            carried_pixel = ((int)i-SDLK_0) * existant_pixeltypes / 10;
                            pixeltype[pixels-1] = carried_pixel;
                        }
                    }
                }
              }
            }
        }
        if (quit_now) break;
        //keybuffer_cleaner();
//            }
//            else

        // apply key hold mechanics

        if (thrust_keyhold) {
            ispd ();
        }
        if (back_keyhold) {
            dspd ();
        }

//no_keys:
        fermo_li = 0;

        if (mpul==0) { // no button
            rclick = -1;
        }
        if (mpul == 1) { // left mouse button
            ispd ();
        }
        if (mpul == 2) { // right mouse button
            dspd ();
            if (!extra) {
                if (rclick==-1)
                    rclick = SDL_GetTicks();
                else {
                    // keep it pressed for half a second to invert
                    if (SDL_GetTicks()-rclick>TICKS_IN_A_SECOND/2) {
                        fid_on (); // invert
                        i = SDLK_F1; //59;
                    }
                }
            }
            extra_stop_extra ();
            if (trackframe) {
                i = 61;
                _x = mx;
                _y = my;
                while (mpul) {
                    mpul = 0;
                    mouse_input();
                }
                mx = _x;
                my = _y;
            }
        }

        // pressing both keys
        if (mpul == 3) {
            if (!trackframe||trackframe==23) {
                undock ();
                i = 62;
            }
            if (extra) {
                if (carry_type==-1)
                    taking = 1;
                else {
                    if (carry_type!=-1)
                        lascia_cadere ();
                }
            }
            _x = mx;
            _y = my;
            while (mpul) {
                mpul = 0;
                mouse_input();
            }
            mx = _x;
            my = _y;
        }

//    nonpul:

        // Calcolo della velocit e della distanza dal Solicchio.
        veloc = sqrt(spd_x*spd_x+spd_y*spd_y+spd_z*spd_z);
        dsol = sqrt(cam_x*cam_x+cam_y*cam_y+cam_z*cam_z);

        if (dsol<15000) {
            tinte (63-dsol/240);
            yel = 1;
        }
        else {
            if (yel) {
                tinte (0);
                yel = 0;
            }
        }

        // Ridimensionamento angolazione verticale in "extra attivit".
        if (extra&&alfa>90&&alfa<270) {
            alfad = 0;
            if (alfa>180)
                alfa = 270;
            else
                alfa = 90;
            my = alfa * 5;
        }

        if (reset_trackframe) {
            tracking = 0;
            trackframe--; spd_y -= 0.1;
            reset_trackframe = trackframe;
        }

        // FID (freno inerziale diamagnetico).
        // Non pi cos facile: ora c' lo SPIN.
        if (fid||lead||orig) {
            alfad = alfa90 - alfa;
            if (alfad>5) alfad = 5;
            if (alfad<-5) alfad = -5;
            alfa += alfad;
            betad = beta90 - beta;
            if (betad>5) betad = 5;
            if (betad<-5) betad = -5;
            beta += betad;
            if (alfa==alfa90&&beta==beta90) {
                m = comera_m;
                mx = beta * 5;
                my = alfa * 5;
                if (orig&&carried_pixel>-1) {
                    carried_pixel--;
                    if (carried_pixel<0) carried_pixel = existant_pixeltypes - 1;
                    pixeltype[pixels-1] = carried_pixel;
                }
                fid = 0;
                lead = 0;
                orig = 0;
                play (TARGET);
            }
            alfad = 0;
            betad = 0;
        }

        obj = -1;

        if (extra) {
            if (fabs(disl)>1)
                rel_y += disl / 3;
            else
                rel_y += disl;
            cam_xt = dsx + rel_x;
            cam_yt = dsy + rel_y;
            cam_zt = dsz + rel_z;
            CollyZone (pix, 0); // Controllo collisioni.
            cam_x = dsx + rel_x;
            cam_y = dsy + rel_y;
            cam_z = dsz + rel_z;
        }

        // Sezione ridisegno spazio illusorio.
        /// Create hallucinating effect.
        if (ctrlkeys[0]&32) { // ctrlkeys[0]&32 is left alt
            darken_once();
                        /*
                        asm {   pusha
                                les di, dword ptr adapted
                                mov cx, 64000 }
                chics:  asm {   cmp byte ptr es:[di], 0
                                je zeros
                                dec byte ptr es:[di] }
                zeros:  asm {   inc di
                                dec cx
                                jnz chics
                                popa }
                        */
        } else {
            rx = pixel_xdisloc[pix] - cam_x;
            ry = pixel_ydisloc[pix] - cam_y;
            rz = pixel_zdisloc[pix] - cam_z;
            d = sqrt (rx*rx + ry*ry + rz*rz);
            if (subsignal[9*pixeltype[pix]]&&d<628) {
                strcpy (dist, &subsignal[9*pixeltype[pix]]);
                strcat (dist, ".ATM");
                a = open (dist, 0);
                if (a>-1) {
//                  asm {
//                  mov bx, a
                    //auto bx = a;
//                  mov ax, 360
                    auto ax = 360u;
//                  sub ax, beta
                    ax -= beta;
//                  mov dx, 32
                    auto dx = 32u;
//                  mul dx          // dx:ax := ax*(dx)
                    ax = ax * dx; // 360*32
//                  xor dx, dx
                    dx = 0;
//                  mov cx, 36
                    auto cx = 36u;
//                  div cx          // ax := (dx:ax) / cx
                    ax = ax / cx; // 360*32/36 = 320
//                  shl ax, 2
                    ax <<= 2; // 320 * 4 = 1280
//                  xor dx, dx
                    dx = 0;
//                  mov cx, 320
                    cx = WIDTH;
//                  div cx
//                  mov si, dx
                    auto si = ax % cx; // si = 1280 % WIDTH
//                  mov ax, alfa
                    ax = alfa;
//                  mov cx, 360
                    cx = 360;
//                  xor dx, dx
                    dx = 0;
//                  div cx  // ax := (dx:ax) / cx
//                  mov ax, dx // dx holds remainder
                    ax = ax % cx; // alfa % 360
//                  mov dx, 3
                    dx = 3;
//                  mul dx // dx:ax := ax*(dx)
                    ax = ax * dx; // (alfa % 360) * 3
//                  xor dx, dx
                    dx = 0;
//                  mov cx, 320
                    cx = WIDTH;
//                  mul cx    // dx:ax := ax*(cx)
                    ax = ax * cx; // ax = (alfa % 360) * 3 * WIDTH
//                  mov cx, dx
                    cx = ax >> 16; // cx = ax >> 16
//                  mov dx, ax
                    dx = ax;
//                  add dx, si
                    dx += si;
//                  jnc add_ok
                    /*
                    if (dx >= 0x10000) {
                        dx &= 0xFFFF;
                        cx ++;
//                  inc cx }
                    } */
//          add_ok:
//                  asm {
//                  mov ax, 0x4200 // move file pointer
//                  int 21h
                    // we'll replace this with a seek operation
                    lseek(a, dx, SEEK_SET);
//                  push ds
//                  lds dx, dword ptr adapted
                    SDL_LockSurface(p_surface);
                    auto p_data = static_cast<unsigned char*>(p_surface->pixels);
//                    p_data += dx;
//                  mov si, 8 }
                    si = 8;
//        atm:
                    do {
//                  asm {
//                  mov cx, 8000
                    cx = WIDTH*HEIGHT / 8;
//                  mov ah, 3fh
//                  int 0x21
                    read(a, p_data, cx);
//                  add dx, 8000
                    p_data += WIDTH*HEIGHT / 8;
//                  dec si
//                  jnz atm
                    } while (--si != 0);
//                  pop ds }
                    SDL_UnlockSurface(p_surface);
                    close (a);
                    if (d>500) {
                        a = (d - 500) / 4;

                        SDL_LockSurface(p_surface);
                        auto si = static_cast<unsigned char*>(p_surface->pixels);
                        unsigned int cx = WIDTH*HEIGHT;
                        do
                        {
                            if (*si < (a&0xFF)) {
                                *si -= a&0xFF;
                                *si = (a>>8)&0xFF;
                            }
                            else *si -= a&0xFF;
                            si++;
                        } while (--cx != 0);
                        SDL_UnlockSurface(p_surface);
//              Old code of the above:
//                asm {   mov ax, a
//                        les si, dword ptr adapted
//                        mov cx, 64000 }
//        diminish: asm { sub es:[si], al
//                        jnc shed
//                        mov es:[si], ah }
//        shed:   asm {   inc si
//                        dec cx
//                        jnz diminish }
                        }
                    } else {
                        SDL_LockSurface(p_surface);
                        pclear (static_cast<unsigned char*>(p_surface->pixels), 0);
                        SDL_UnlockSurface(p_surface);
                    }
            }
            else {
                SDL_LockSurface(p_surface);
                pclear (static_cast<unsigned char*>(p_surface->pixels), 0);
                SDL_UnlockSurface(p_surface);
            }
        }

        // Disegno oggetto prelevato.

        if (carry_type!=-1) {
            id = 0;
            if (trackframe==23) {
                cox += pixel_xdisloc[pix] - prevpixx;
                coz += pixel_zdisloc[pix] - prevpixz;
            }

            coy += ((cam_y + 9 * tsin[alfa]) - coy) / justloaded;
            cox += ((cam_x - 9 * tsin[beta] * tcos[alfa]) - cox) / justloaded;
            coz += ((cam_z + 9 * tcos[beta] * tcos[alfa]) - coz) / justloaded;
            if (trackframe==23&&!memcmp(&subsignal[9*(carry_type+FRONTIER_M3)], "MAGNIFY" /*"VISORE"*/, 7/*6*/)) {
                cam_x = cox;
                cam_y = coy;
                cam_z = coz;
            }
            else {
                ox = cox; oy = coy; oz = coz;
                Object (carry_type);
            }
        }

        // Gestione pixels "donati" dal Solicchio.
        if (dsol<500)
            if (pixels<500&&carried_pixel==-1) {
                carried_pixel = existant_pixeltypes - 1;
                pixeltype[pixels] = carried_pixel;
                pixel_support[pixels] = 0;
                pixel_rot[pixels] = 0;
                pixels++;
            }

        if (carried_pixel>-1) {
            pixel_ydisloc[pixels-1] = cam_y + 1000 * tsin[alfa];
            pixel_xdisloc[pixels-1] = cam_x - 1000 * tsin[beta] * tcos[alfa];
            pixel_zdisloc[pixels-1] = cam_z + 1000 * tcos[beta] * tcos[alfa];
            if (veloc<2&&dsol>500&&!trackframe) carried_pixel = -1;
        }

        for (a=(int)(SDL_GetTicks()%18); a<180+(int)(SDL_GetTicks()%18); a+=18) // "Sole" centrale (il Solicchio).
            for (b=(int)(SDL_GetTicks()%18); b<180+(int)(SDL_GetTicks()%18); b+=18) {
                ox = random (14000) + 1000;
                oz = ox * tcos[b];
                z2 = ox * tsin[b];
                ox = z2 * tcos[a];
                oy = z2 * tsin[a];
                Line3D (-ox, -oz, oy, ox, oz, -oy);
                ox = random (7000) + 500;
                oz = ox * tcos[b];
                z2 = ox * tsin[b];
                ox = z2 * tcos[a];
                oy = z2 * tsin[a];
                Line3D (-ox, oy, -oz, ox, -oy, oz);
            }

        vicini = 0;

        for (p=0; p<pixels; p++) { // Disegna tutti i pixels.
            nopix = p;
            Pixel (pixeltype[p]);
            Oggetti_sul_Pixel (0);
        }

        if (globalvocfile[0]!='.') {
            nopix = pixel_sonante;
            Oggetti_sul_Pixel (1);
        }

        if (!vicini&&globalvocfile[0]!='.') chiudi_filedriver ();

        if (trackframe==23)
            justloaded = 5;
        else
            justloaded = 1;

        for (o=0; o<_objects; o++) { // Per gli oggetti vagabondi.
            if (object_location[o]==-1) {
                ox = absolute_x[o];
                oy = absolute_y[o];
                oz = absolute_z[o];
                xrel (0, 0, 0);
                kk = sqrt (ox*ox+oy*oy+oz*oz);
                if (kk<15000||kk>3.3333333E8) {
                    /* Assorbiti dal Solicchio o usciti dal
                    campo di definizione (10000 Km.) */
                    scoppia (o, 10000, 1);
                    o--;
                }
                else {
                    rx = ox - cam_x;
                    ry = oy - cam_y;
                    rz = oz - cam_z;
                    kk = sqrt (rx*rx+ry*ry+rz*rz);
                    //if (kk<pixel_absd[pix]) gap = kk / 500 + 5;
                    if (kk<1000) Object (objecttype[o]);
                    absolute_x[o] += relative_x[o];
                    absolute_y[o] += relative_y[o];
                    absolute_z[o] += relative_z[o];
                }
            }
        }

/*              if (sc) {
                        rx = (double)alfa*Deg + micro_x;
                        ry = (double)beta*Deg + micro_y;
                        cam_y -= sc * sin(rx);
                        cam_x += sc * sin(ry) * cos(rx);
                        cam_z -= sc * cos(ry) * cos(rx);
                        sc = 0;
                } */

        if (taking&&!extra) {
            kk = 500;
            obj = -1;
            for (o=_objects-1; o>=0; o--) {
                if (object_location[o]==-1) {
                    _x = absolute_x[o] - cam_x;
                    _y = absolute_y[o] - cam_y;
                    _z = absolute_z[o] - cam_z;
                    d = sqrt (_x*_x+_y*_y+_z*_z);
                    if (d<kk) {
                        kk = d;
                        obj = o;
                    }
                }
            }
        }

        if (taking&&obj>-1) {
            coy = pixel_ydisloc[pix] + relative_y[obj] - object_elevation[objecttype[obj]];
            cox = pixel_xdisloc[pix] + relative_x[obj];
            coz = pixel_zdisloc[pix] + relative_z[obj];
            preleva_oggetto (obj);
        }
        taking = 0;

        // Disegno navicella e sua gestione.
   // Disegno navicella e sua gestione.
    if (!extra) {
        nav_x = cam_x;
        nav_y = cam_y;
        nav_z = cam_z;
        nav_a = 360 - alfa;
        if (nav_a==360) nav_a=0;
        nav_b = 360 - beta;
        if (nav_b==360) nav_b=0;
    }
    else {
        nav_x = pixel_xdisloc[pix] + docksite_x[pixeltype[pix]];
        nav_z = pixel_zdisloc[pix] + docksite_z[pixeltype[pix]];
        nav_y = pixel_ydisloc[pix] + docksite_y[pixeltype[pix]] - 16;
    }

    if (req_end_extra) {
        if (req_end_extra<50) req_end_extra++;
        if (alfa>179) alfa-=360;
        alfa = (double) alfa / 1.1;
        if (alfa<0) alfa+=360;
        auto p = ((360-nav_b) - beta) / 4;
        if (p)
            beta += (double) p;
        else
            beta = 360 - nav_b;
        //if (beta<0) beta+=360;
        //if (beta>359) beta-=360;
        alfad = 0;
        betad = 0;
        rel_x /= 1.25; rel_z /= 1.25;
        if (alfa==0&&beta==360-nav_b
                &&(rel_x<1&&rel_x>-1)
                &&(rel_z<1&&rel_z>-1)
                &&req_end_extra>=24) {
            play (READY);
            subs = 0;
            req_end_extra = 0;
            extra = 0;
        }
    }

    if (tracking)
        trackframe += tracking;

    if (echo&8) goto no_ind;

    // Indicatori direzionali.
    if (!trackframe) {
        n (-1, -1, 18, -0.5, -0.5, 18); // "Mirino".
        n (-1, 1, 18, -0.5, 0.5, 18);
        n (1, -1, 18, 0.5, -0.5, 18);
        n (1, 1, 18, 0.5, 0.5, 18);
        for (c=0; c<2; c++) {
            if (C32(nav_x+spd_x, nav_y+spd_y, nav_z+spd_z)) {
                if (share_x>5&&share_x<315&&share_y>5&&share_y<195) {
                    Segmento (share_x - 5, share_y - 5, share_x + 5, share_y + 5);
                    Segmento (share_x + 5, share_y - 5, share_x - 5, share_y + 5);
                }
            }
            else {
                if (C32(nav_x-spd_x, nav_y-spd_y, nav_z-spd_z)) {
                    if (share_x>10&&share_x<310&&share_y>10&&share_y<190) {
                        Segmento (share_x - 9, share_y - 5, share_x + 9, share_y - 5);
                        Segmento (share_x - 9, share_y + 5, share_x + 9, share_y + 5);
                        Segmento (share_x - 5, share_y - 9, share_x - 5, share_y + 9);
                        Segmento (share_x + 5, share_y - 9, share_x + 5, share_y + 9);
                    }
                }
            }
            if (pixel_absd[pix]>8000) {
                if (C32(pixel_xdisloc[pix], pixel_ydisloc[pix], pixel_zdisloc[pix])) {
                    if (share_x>20&&share_x<300&&share_y>20&&share_y<180) {
                        Segmento (share_x, share_y - 19, share_x, share_y + 19);
                        Segmento (share_x - 19, share_y, share_x + 19, share_y);
                    }
                }
            }
        }
    }

    no_ind: if (echo&2) goto no_nav;

    if (!extra) {
        tsinx += 361;
        tcosx += 361;
        tsiny += 361;
        tcosy += 361;
    }

    // Pannello informativo.
    nrect (0, 4.95, 12.01, 6, 0.95);

    nrect (0, 4.95, 14, 6, 0.95);
    n (-6, 4, 12.01, -6, 4, 14);
    n ( 6, 4, 12.01,  6, 4, 14);
    n (-6, 5.9, 12.01, -6, 5.9, 14);
    n ( 6, 5.9, 12.01,  6, 5.9, 14);

    // Parte anteriore della navicella.
    nrect (0, 13.5, 58, 4, 2.5);

    n (-10, 16, 16,  10, 16, 16);
    n (-10, -5, 16, -10, 16, 16);
    n ( 10, -5, 16,  10, 16, 16);
    n (-10,  1, 25,  -4, 11, 58); //
    n (-10,  1, 25, -10, -5, 16); ////
    n (-10,  1, 25,-8.7, 16, 25); //////
    //n ( -4, 11, 58,   4, 11, 58);
    //n (  4, 11, 58,   4, 16, 58);
    //n ( -4, 11, 58,  -4, 16, 58);
    //n (  4, 16, 58,  -4, 16, 58);
    n (  4, 11, 58,  10,  1, 25); //
    n ( 10,  1, 25,  10, -5, 16); ////
    n ( 10,  1, 25, 8.7, 16, 25); //////
    n (  4, 16, 58,  10, 16, 16);
    n ( -4, 16, 58, -10, 16, 16);
    n ( -4, 16, 58,   0, 16, 83);
    n (  0, 16, 83,   4, 16, 58);
    n (  0, 16, 83,  -4, 11, 58);
    n (  0, 16, 83,   4, 11, 58);
    n (  0, 13, 64,  -4, 11, 58);
    n (  0, 13, 64,   4, 11, 58);
    n (  0, 13, 64,  -4, 16, 58);
    n (  0, 13, 64,   4, 16, 58);

    if (extra) {
        // Attracchi.

/*                  _x = pixel_xdisloc[pix]+docksite_x[pixeltype[pix]];
                    _y = pixel_ydisloc[pix]+docksite_y[pixeltype[pix]];
                    _z = pixel_zdisloc[pix]+docksite_z[pixeltype[pix]];

                    Line3D (nav_x-10, nav_y-5, nav_z+16, _x-50, _y, _z+50);
                    Line3D (nav_x+10, nav_y-5, nav_z+16, _x+50, _y, _z+50);
                    Line3D (nav_x-14, nav_y-10, nav_z-31, _x-50, _y, _z-50);
                    Line3D (nav_x+14, nav_y-10, nav_z-31, _x+50, _y, _z-50); */

        // Cabina.
        n (-10,  -5,  16, -14, -10, -31);
        n ( 10,  -5,  16,  14, -10, -31);
        n (-10,  16,  16, -14,  16, -31);
        n ( 10,  16,  16,  14,  16, -31);
        n (-14, -10, -31,  14, -10, -31);
        n (-14, -10, -31, -10,   0, -43);
        n ( 14, -10, -31,  10,   0, -43);
        n (  0,  16, -69, -10,   0, -43);
        n (  0,  16, -69,  10,   0, -43);
        n (  0,  16, -69, -10,  16, -43);
        n (  0,  16, -69,  10,  16, -43);
        n (-10,  16, -43, -14,  16, -31);
        n ( 10,  16, -43,  14,  16, -31);
        n (-10,   0, -43,  10,   0, -43);
        n (-10,  16, -43,  10,  16, -43);
        n (-10,   0, -43, -10,  16, -43);
        n ( 10,   0, -43,  10,  16, -43);
        n (-14, -10, -31, -14,  16, -31);
        n ( 14, -10, -31,  14,  16, -31);

        // Cupola della cabina.
        n (-10,  -5,  16,  -6,  -9,   8);
        n ( -6,  -9,   8,   6,  -9,   8);
        n (  6,  -9,   8,  10,  -5,  16);
        n ( -6,  -9,   8, -10, -14, -26);
        n (-10, -14, -26, -14, -10, -31);
        n (-10, -14, -26,  10, -14, -26);
        n (  6,  -9,   8,  10, -14, -26);
        n ( 10, -14, -26,  14, -10, -31);

        // Ali.
        n (-10, 16,  16, -40, 16, -55);
        n (-40, 16, -55, -10, 16, -43);
        n ( 10, 16,  16,  40, 16, -55);
        n ( 40, 16, -55,  10, 16, -43);

    }

    // Sezione ridisegno pannello di controllo informativo.
    ox = nav_x; oy = nav_y; oz = nav_z;

    sprintf (dist, "F=%04d/CGR", (int)spd);
    nTxt (dist, -5.5, 4.5, 12.01, 0.07, 0.12);

    if (!trackframe) sprintf (dist, "V=%04.0f:K/H", (float)veloc*1.9656);
    else sprintf (dist, "--DOCKED--" /*"ATTRACCATO"*/);
    nTxt (dist, -5.5, 5.4, 12.01, 0.07, 0.12);

    sprintf (dist, "ASC=%03d`", nav_b);
    nTxt (dist, -2.25, 4.5, 12.01, 0.07, 0.12);
    sprintf (dist, "DEC=%03d`", nav_a);
    nTxt (dist, -2.25, 5.4, 12.01, 0.07, 0.12);

    sprintf (dist, "H=%1.1f:KM", -cam_y/33333.33333);
    nTxt (dist, 0.35, 4.5, 12.01, 0.06, 0.12);
    kk = pixel_absd[pix]/33.33333333;
    if (kk>1000)
        sprintf (dist, "D=%1.1f:KM", kk/1000);
    else
        sprintf (dist, "D=%1.1f:MT", kk);
    nTxt (dist, 0.35, 5.4, 12.01, 0.06, 0.12);

    sprintf (dist, "X=%1.1f:KM", cam_x/33333.33333);
    nTxt (dist, 3.1, 4.5, 12.01, 0.07, 0.12);
    sprintf (dist, "Z=%1.1f:KM", cam_z/33333.33333);
    nTxt (dist, 3.1, 5.4, 12.01, 0.07, 0.12);

    // Disegno tasti funzione, interruttori e/m, ecc...
    if (!trackframe) {
        nTxt ("N", -4, -4.5, 12.01, 0.07, 0.07);
        nTxt ("S", -4, -3.5, 12.01, 0.07, 0.07);
        nTxt ("E", -3.5, -4, 12.01, 0.07, 0.07);
        nTxt ("O", -4.5, -4, 12.01, 0.07, 0.07);
        n (-4-tcos[nav_b+270]/2, -4-tsin[nav_b+270]/2, 12.01, -4+tcos[nav_b+270], -4+tsin[nav_b+270], 12.01);
        n (-4-tcos[nav_b+180]/2, -4-tsin[nav_b+180]/2, 12.01, -4+tcos[nav_b+180]/2, -4+tsin[nav_b+180]/2, 12.01);
        nTxt ("Z", 4, -4.5, 12.01, 0.07, 0.07);
        nTxt ("N", 4, -3.5, 12.01, 0.07, 0.07);
        nTxt ("-", 4.5, -4, 12.01, 0.07, 0.07);
        if (alfa>90&&alfa<270&&blink) nTxt ("*", 3.5, -4, 12.01, 0.07, 0.07);
        alfax = 359 - nav_a;
        n (4-tcos[alfax]/2, -4-tsin[alfax]/2, 12.01, 4+tcos[alfax], -4+tsin[alfax], 12.01);
        n (4-tcos[alfax+90]/2, -4-tsin[alfax+90]/2, 12.01, 4+tcos[alfax+90]/2, -4+tsin[alfax+90]/2, 12.01);
    }

    bki9 = bki;
    bki0 = bki;
    bki1 = bki;
    bki2 = bki;
    bki3 = bki;

    if (!trackframe&&!extra&&i==59) bki9 = i;
    if (!extra&&i==60&&!trackframe) bki0 = i;
    if (trackframe&&i==61) bki1 = i;
    if (trackframe&&!extra&&i==62) bki2 = i;
    if (orig&&i==63) bki3 = i;

        console_key ("SPIN", -6.0, 59, i, i, bki9);
        console_key ("LEAD", -4.9, 60, i, i, bki0);
        console_key ("EXTR", -3.8, 61, i, i, bki1);
        console_key ("DOCK", -2.7, 62, i, i, bki2);
        console_key ("ORIG", -1.6, 63, i, i, bki3);

        console_key ("ECHO", 5.0, echo&1, 1, echo, bkecho);

        // end drawing the fly

        bki = i;
        bkecho = echo;

        if (!extra) {
            tsinx -= 361;
            tcosx -= 361;
            tsiny -= 361;
            tcosy -= 361;
        }

        // Esplosioni: Ragassi, esplodete gli oggetti... per favore!
        no_nav:
            if (!explode_count) {
                if (!trackframe) {
                    for (o=0; o<_objects; o++) {
                        if (object_location[o]==-1&&!memcmp(&subsignal[9*(objecttype[o]+FRONTIER_M3)], "BOMB", 4)) {
                            rx = pixel_xdisloc[pix] - absolute_x[o];
                            ry = pixel_ydisloc[pix] - absolute_y[o];
                            rz = pixel_zdisloc[pix] - absolute_z[o];
                            if (sqrt(rx*rx+ry*ry+rz*rz)<500) {
                                explode_count = 100;
                                scoppia (o, 5000, 1);
                                break;
                            }
                        }
                    }
                }
            }
            else {
                explode_count--;
                if (explode_count == 60) {
                    play (DESTROY);
                    subs = 0;
                }
                if (!explode_count) {
                    explode = 0;
                    if (pixel_sonante==pix)
                        chiudi_filedriver ();
                    else {
                        if (pixel_sonante>pix)
                            pixel_sonante--;
                    }
                    for (o=0; o<_objects; o++) {
                        if (object_location[o]==pix) {
                            absolute_x[o] = pixel_xdisloc[pix] + relative_x[o];
                            absolute_y[o] = pixel_ydisloc[pix] + relative_y[o];
                            absolute_z[o] = pixel_zdisloc[pix] + relative_z[o];
                            //adapted = adaptor;
                            scoppia (o, (double)(random(10)+10)*object_elevation[objecttype[o]] + 50, 5);
                            // perform a frame skipping of some sort
//                    asm {   pusha
//                            xor ah, ah
//                            int 0x1a
//                            mov si, dx }
                            auto si = SDL_GetTicks();
//            ky: asm {       xor ah, ah
//                            int 0x1a
//                            cmp si, dx
//                            je ky
                            while (si + TICKS_PER_FRAME > SDL_GetTicks());
//                            popa }
                            //adapted = fake_adaptor;
                            //pcopy (adaptor, adapted);
                            o--;
                        }
                    }
                    for (o=0; o<_objects; o++) {
                        if (object_location[o]>pix)
                            object_location[o]--;
                    }
                    for (c=pix; c<pixels; c++) {
                        pixeltype[c] = pixeltype[c+1];
                        pixel_rot[c] = pixel_rot[c+1];
                        pixel_absd[c] = pixel_absd[c+1];
                        pixel_support[c] = pixel_support[c+1];
                        pixel_xdisloc[c] = pixel_xdisloc[c+1];
                        pixel_ydisloc[c] = pixel_ydisloc[c+1];
                        pixel_zdisloc[c] = pixel_zdisloc[c+1];
                    }
                    pixels--;
                }
            }

            // Cosette finali.
            /* Sound Off for now
            if (!sbp_stat&&!sbf_stat) { // Gestione audio in sottofondo...
                subs = 1;
                if (globalvocfile[0]!='.') {
                    chiudi_filedriver ();
                    for (o=0; o<_objects; o++) {
                        if (object_location[o]==pixel_sonante)
                            if (absolute_y[o]==11E11)
                                absolute_y[o] = 0;
                    }
                    // Per poter
                    // riprodurre registrazioni
                    // in modo ciclico.
                    nopix = pixel_sonante;
                    // Per poter
                    // rogrammare
                    // sequenze audio.
                    Oggetti_sul_Pixel (1);
                }
            }
            */

            /* Sound stuff
            if (!extra&&!trackframe) { // Segnale dell'ecoscandaglio.
                if (SDL_GetTicks()-stso>=gap) {
                    stso = SDL_GetTicks();
                    if (subs&&(echo&1)) play (SOTTOFONDO);
                }
            }
            */

            if (moving_last_object) { // "Adagia" l'oggetto lasciato.
                _x = cfx - relative_x[_objects-1];
                _y = cfy - relative_y[_objects-1];
                _z = cfz - relative_z[_objects-1];
                relative_x[_objects-1] += _x / 5;
                relative_y[_objects-1] += _y / 5;
                relative_z[_objects-1] += _z / 5;
                if (sqrt(_x*_x+_y*_y+_z*_z)<0.1) {
                    moving_last_object = 0;
                    relative_x[_objects-1] = cfx;
                    relative_y[_objects-1] = cfy;
                    relative_z[_objects-1] = cfz;
                    if (globalvocfile[0]=='.') play (LASCIARE);
                }
            }

            //SDL_Flip(p_surface);
            Render();
            //pcopy (adaptor, adapted); // Il "display-swapping" a 32 bit.

            blink = 1 - blink; // Lampeggo della spia "volo rovesciato".

            unsigned long cticks;
            do {
                cticks = SDL_GetTicks();
                SDL_Delay(10);
            } while (/*blink && */ (sync + TICKS_PER_FRAME > cticks));
            //while (blink && (clock()==sync)); // Sincronizzatore (max 37 fps.)

            //cout << " " << (TICKS_IN_A_SECOND / (cticks-sync)) << " fps" << endl;
            //cout << "\t\t" << (cticks-sync) << "\t\t\r"; cout.flush();
    } while (i!=27);

    alfin (1);

    cout << "Quitting." << endl;
    return 0;
}

inline void init_start()
{
    int r;
    initscanlines ();
    init ();

    auto mem_ok = allocation_farm();
    if (!mem_ok) {
        cerr << "Not enough memory (seriously!?)" << endl;
        throw 1;
    }

	// Init SDL
	r = SDL_Init( SDL_INIT_TIMER | SDL_INIT_VIDEO);
	if (r < 0)
	{	cerr << "Failed to init SDL: " << SDL_GetError() << endl;
		throw 2;
    }

    atexit(SDL_Quit);
}

void read_args(int argc, char** argv, char& flag, char& sit)
{
    flag = 0;
    char dist[20]; // Stringhe usate per conversioni.
    if (argc>2) {
        pixels = objects = 0;
        if (strcasecmp(argv[2], "PIXELS") == 0) {
            pixels = (int)atof(argv[1]);
            if (pixels<1) pixels = 1;
            if (pixels>500) pixels = 500;
        }
        if (argc > 4 && strcasecmp(argv[4], "OBJECTS") == 0) {
                objects = (int)atof(argv[3]);
                if (objects<1) objects = 1;
                if (objects>500) objects = 500;
        }
        if (objects==0||pixels==0) {
            _80_25_C();
            cerr << "Usage: CRYXTELS ([<n> PIXELS <n> OBJECTS] | <gamesave>)" << endl;
            throw 1;
        }
    }
    else if (argc == 2) {
        if (argc==2) sit = argv[1][0];
        sprintf (dist, "CRYXTELS.%cIT", sit);
        if ((fh = open (dist, 0)) >= 0) {
            close (fh);
            flag = 1;
        } else {
            _80_25_C();
            cerr << "Could not load game: game save \"" << dist << "\" is missing." << endl;
            throw 1;
        }
    }
    else {
        // Use default pixel and object numbers
        pixels = 250;
        objects = 250;
    }
}


void par0 (int el, int pix)
{
    alfin (0);
    fprintf (stderr, "Nel pixel di tipo %d, elemento nr. %d,  stato specificato", pix, el+1);
    fprintf (stderr, "\nun parametro, pari a 0, non valido in quel punto.\n");
    exit (1);
}

template <class T>
T* x_farmalloc (unsigned long nelems)
{
    T * fp = new T[nelems];
    if (!fp) {
        throw 0;
    }

    memset (fp, 0, nelems*sizeof(T));

    return fp;
}

inline bool allocation_farm()
{
    try {
    pixel_elem_b = x_farmalloc<char>(40*ELEMS*BUFFERS);

    pixeltype = x_farmalloc<short> (500);
    pixelmass = x_farmalloc<float> (650); // 2600
    pixel_rot = x_farmalloc<unsigned char>(500 * 2);

    pixel_absd = x_farmalloc<float>(500);
    pixel_support = x_farmalloc<double> (500);
    pixel_xdisloc = x_farmalloc<double> (500);
    pixel_ydisloc = x_farmalloc<double> (500);
    pixel_zdisloc = x_farmalloc<double> (500);

    objecttype = x_farmalloc<short> (500);
    relative_x = x_farmalloc<double> (500);
    relative_y = x_farmalloc<double> (500);
    relative_z = x_farmalloc<double> (500);
    absolute_x = x_farmalloc<double> (500);
    absolute_y = x_farmalloc<double> (500);
    absolute_z = x_farmalloc<double> (500);
    object_location = x_farmalloc<short> (500);

    pixeltype_elements = x_farmalloc<unsigned char>(BUFFERS);
    pixeltype_type = x_farmalloc<short> (BUFFERS);

    pixel_elem_t = x_farmalloc<unsigned char> (ELEMS*BUFFERS);
    pixel_elem_x = x_farmalloc<double> (ELEMS*BUFFERS);
    pixel_elem_y = x_farmalloc<double> (ELEMS*BUFFERS);
    pixel_elem_z = x_farmalloc<double> (ELEMS*BUFFERS);
    pixel_elem_1 = x_farmalloc<float> (ELEMS*BUFFERS);
    pixel_elem_2 = x_farmalloc<float> (ELEMS*BUFFERS);
    pixel_elem_3 = x_farmalloc<float> (ELEMS*BUFFERS);
    pixel_elem_4 = x_farmalloc<float> (ELEMS*BUFFERS);

    docksite_x = x_farmalloc<float>(650); // For 650 pixel types.
    docksite_y = x_farmalloc<float>(650);
    docksite_z = x_farmalloc<float>(650);
    docksite_w = x_farmalloc<float>(650);
    docksite_h = x_farmalloc<float>(650);
    subsignal = x_farmalloc<char> (5850);

    buffer = x_farmalloc<unsigned char> (1057);
    } catch (const int&) {
        return false;
    }
    return true;
}

/*
unsigned int clock ()
{
    return SDL_GetTicks();
}*/

void tinte (char satu)
{
    constexpr unsigned char K = 255;
    constexpr float F1 = 255.f / 48; // 1.333333;
    constexpr float F2 = 255.f / 48; // 1.333333;
    int i;
        for (i=0; i<768; i++) buffer[i] = K;
        for (i=0; i<48; i+=3) {
                buffer[i] = buffer[i+1] = satu;
                buffer[i+2] = (float)i*F1;
        }
        for (i=0; i<48; i+=3) {
                int v = i*F2 + satu;
                if (v > K) v = K;
                buffer[i+48] = v;
                buffer[i+49] = buffer[i+48];
                buffer[i+50] = K;
        }
        tavola_colori (buffer, 0, 256, 63, 63, 63);
}

/// redefinition of random(int),
/// which probably became deprecated.
/// Generates a random number between 0 and max_num-1
int random(int max_num) {
    int result, hi_num=max_num;
    result = (rand()%(hi_num));
    assert(result >= 0 && result < max_num);
    return result;
}

void build_cosm(char& flag)
{
    int p;

    /* Costruisce il microcosmo, assegnando una posizione, casuale ma costante,
        ad ogni pixel e ad ogni oggetto. Inoltre, vengono scelte le
        forme di ogni corpo del microcosmo. */

    for (p=0; p<500; p++) {
            //pixelmass[p] = 0;
            pixeltype[p] = p % existant_pixeltypes;
            pixel_xdisloc[p] = (double)(random(10000) - random(10000)) * 150.0;
            pixel_ydisloc[p] = (double)(random(1000) - 500) * 150.0;
            //pixel_support[p] = (double)(random(1000)) / 200;
            pixel_zdisloc[p] = (double)(random(10000) - random(10000)) * 150.0;
            //pixel_absd[p] = 0;
            pixel_rot[p] = 0;
    }

    //ctk = ctrlkeys[0];

    cout << "Loading pixels..." << endl;
    for (p=0; p<existant_pixeltypes; p++) {
        cout << "Percentage complete: " << 100*(long)p/existant_pixeltypes << "\r";
        loaded_pixeltypes = 0; LoadPtyp (p);
    }
    cout << "Percentage complete: 100";
    cout << "\n\nLoading objects..." << endl;
    pixelmass[FRONTIER_M3] = 100; // Massa del fottifoh.
    pixelmass[FRONTIER_M2] = 100; // del motore orbitale.
    pixelmass[FRONTIER_M1] = 100; // del lettore di cd.
    for (p=3; p<existant_objecttypes; p++) {
        cout << "Percentage complete: " << 100*(long)p/existant_objecttypes << "\r";
        loaded_pixeltypes = 0; LoadPtyp (p+FRONTIER_M3);
    }
    cout << "Percentage complete: 100\n" << endl;

    if (!flag) {
        //ctrlkeys[0] = 0;
        dropping = 1;
        extra = 1;
        _objects = 0;
        cout << "Object positioning taking place." << endl;
        for (int o=0; o<objects; o++) {
            cout << "Percentage complete: "
                << (100*o/objects) << "\r";
            object_location[o] = random (pixels);
            loaded_pixeltypes = 0;
            LoadPtyp (pixeltype[object_location[o]]);
            objecttype[o] = o % existant_objecttypes;
            relative_x[o] = random (docksite_w[pixeltype[object_location[o]]])
                    - random (docksite_w[pixeltype[object_location[o]]]);
            if (docksite_h[pixeltype[object_location[o]]]>=0) {
                relative_z[o] = random (docksite_h[pixeltype[object_location[o]]])
                        - random (docksite_h[pixeltype[object_location[o]]]);
            } else {
                relative_x[o] /= 1.4142;
                relative_z[o] = random (docksite_w[pixeltype[object_location[o]]])
                        - random (docksite_w[pixeltype[object_location[o]]]);
                relative_z[o] /= 1.4142;
            }
            rel_y = 0;
            rel_x = relative_x[o];
            rel_z = relative_z[o];
            CollyZone (object_location[o], 1);
            if (onblock==2) {
                o--;
                continue;
            }
            if (objecttype[o]==1) pixel_rot[object_location[o]]++;
            if (!onblock)
                relative_y[o] = docksite_y[pixeltype[object_location[o]]];
            else
                relative_y[o] = rel_y;
            _objects++;
        }
        cout << "Percentage complete: 100" << endl;

        dropping = 0;
        extra = 0;
    }

}

void Archivia_Situazione (char i)
{
    int fh;
    unsigned conta;
    unsigned char a;

    if (moving_last_object) return;

    objects = _objects;

    if (i >= 'a' && i <= 'z') {
        i -= 'a' - 'A';
    }

    sprintf (t, "CRYXTELS.%cIT", i);
    fh = creat (t, 0666);
    if ( fh != -1) {
        write (fh, &pixels, sizeof(short));
        write (fh, &pixel_support[0], sizeof(double)*pixels);
        write (fh, &pixel_xdisloc[0], sizeof(double)*pixels);
        write (fh, &pixel_ydisloc[0], sizeof(double)*pixels);
        write (fh, &pixel_zdisloc[0], sizeof(double)*pixels);
        write (fh, &objects, sizeof(short));
        write (fh, &objecttype[0], sizeof(short)*objects);
        write (fh, &relative_x[0], sizeof(double)*objects);
        write (fh, &relative_y[0], sizeof(double)*objects);
        write (fh, &relative_z[0], sizeof(double)*objects);
        write (fh, &absolute_x[0], sizeof(double)*objects);
        write (fh, &absolute_y[0], sizeof(double)*objects);
        write (fh, &absolute_z[0], sizeof(double)*objects);
        write (fh, &object_location[0], sizeof(short int)*objects);
        write (fh, &cam_x, sizeof(double));
        write (fh, &cam_y, sizeof(double));
        write (fh, &cam_z, sizeof(double));
        write (fh, &alfa, sizeof(short));
        write (fh, &beta, sizeof(short));
        write (fh, &nav_a, sizeof(short));
        write (fh, &nav_b, sizeof(short));
        write (fh, &taking, sizeof(char));
        write (fh, &carry_type, sizeof(short));
        write (fh, &trackframe, sizeof(double));
        write (fh, &reset_trackframe, sizeof(char));
        write (fh, &tracking, sizeof(double));
        write (fh, &req_end_extra, sizeof(char));
        write (fh, &alfad, sizeof(short));
        write (fh, &betad, sizeof(short));
        write (fh, &pix, sizeof(short));
        write (fh, &alfa90, sizeof(short));
        write (fh, &beta90, sizeof(short));
        write (fh, &fid, 1);
        write (fh, &lead, 1);
        write (fh, &orig, 1);
        write (fh, &comera_m, 1);
        write (fh, &spd_x, sizeof(double));
        write (fh, &spd_y, sizeof(double));
        write (fh, &spd_z, sizeof(double));
        write (fh, &spd, sizeof(double));
        write (fh, &extra, 1);
        write (fh, &rel_x, sizeof(double));
        write (fh, &rel_y, sizeof(double));
        write (fh, &rel_z, sizeof(double));
        write (fh, &obj, sizeof(short));
        write (fh, &m, 1);
        write (fh, &echo, 1);
        write (fh, &carried_pixel, sizeof(short));
        write (fh, &disl, sizeof(double));
        write (fh, &cursore, sizeof(short));
        write (fh, &explode_count, 1);
        a = ctrlkeys[0]; if (a&2) a ^= 2;
        write (fh, &a, 1);
        write (fh, &pixel_rot[0], pixels);
        write (fh, &pixeltype[0], sizeof(short)*pixels);
        write (fh, &repeat, 1);
        write (fh, &source, 1);
        conta = write (fh, &quality, 1);
        if (conta!=1) {
            close (fh);
            remove (t);
            return;
        }
        close (fh);
        cout << "Game [" << i << "] successfully saved." << endl;
    } else {
        int err = errno;
        cerr << "Failed to save game to \"" << t << "\": " << strerror(err) << endl;
    }
}

void rot ()
{
    // Assegnazione posizioni a pixels gravitanti.

    prevpixx = pixel_xdisloc[pix];
    prevpixz = pixel_zdisloc[pix];

    for (c=0; c<pixels; c++) {
        if (pixel_rot[c]) {
            _ox = pixel_xdisloc[c];
            _oz = pixel_zdisloc[c];
            rx = sqrt (_ox*_ox + _oz*_oz);
            pixel_support[c] -= (double) pixel_rot[c] * (50.85 / rx);
            pixel_xdisloc[c] = rx * cos (pixel_support[c]);
            pixel_zdisloc[c] = rx * sin (pixel_support[c]);
            _x = pixel_xdisloc[c] - _ox;
            _z = pixel_zdisloc[c] - _oz;
            if (sqrt(_x*_x+_z*_z)>rx/2) {
                pixel_support[c] += Pi;
                pixel_xdisloc[c] = rx * cos (pixel_support[c]);
                pixel_zdisloc[c] = rx * sin (pixel_support[c]);
            }
        }
    }
}

void fade ()
{
    keybuffer_cleaner ();
//rip:
    unsigned int dx = 0;
    auto skip = false;
    unsigned int sync;
    do {
        sync = SDL_GetTicks();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN
                || event.type == SDL_MOUSEBUTTONDOWN) {
                skip = true;
            }
        }

        darken_once();
        Render();
        //SDL_Flip(p_surface);
        while (sync + TICKS_PER_FRAME > SDL_GetTicks());
    }
    while(!skip && dx++ < 100);
/*
rip:    mpul = 0; mouse_input ();
        _BL = tasto_premuto ();
        asm {   les di, dword ptr adaptor
                mov cx, 64000
                xor dx, dx }
__chic: asm {   cmp byte ptr es:[di], 0
                je __zero
                dec byte ptr es:[di]
                inc dx }
__zero: asm {   inc di
                dec cx
                jnz __chic
                cmp bl, 0
                jne halt
                cmp mpul, 0
                jne halt
                cmp dx, 100
                jnb rip }
halt:   keybuffer_cleaner ();
*/
}

void cerca_e_carica (int typ)
{
    int iii;

    if (loaded_pixeltypes) {
        for (iii = 0; iii<loaded_pixeltypes; iii++) {
            if (typ==pixeltype_type[iii])
                return;
        }
    }
    LoadPtyp (typ);
}

void Aggiornamento ()
{
    int p, o, f;

    loaded_pixeltypes = 0;

    for (p=0; p<pixels; p++) {
        if (pixel_absd[p]<pixelmass[pixeltype[p]])
            cerca_e_carica (pixeltype[p]);
    }

    for (o=0; o<_objects; o++) {
        if (objecttype[o]>=3) {
                f = 0;
                if (object_location[o]>-1) {
                        if (pixel_absd[object_location[o]]<4000)
                            f = 1;
                }
                else {
                    rx = absolute_x[o] - cam_x;
                        ry = absolute_y[o] - cam_y;
                        rz = absolute_z[o] - cam_z;
                        if (sqrt(rx*rx+ry*ry+rz*rz)<1000) f = 1;
                        }
                        if (f) cerca_e_carica (objecttype[o]+FRONTIER_M3);
                }
        }

        if (carry_type>=3)
                cerca_e_carica (carry_type+FRONTIER_M3);
}

void dists ()
{
    int p;

    double related_x; // Usate nel calcolo delle distanze dei pixels.
    double related_y;
    double related_z;

    for (p=0; p<pixels; p++) {
            related_x = pixel_xdisloc[p] - cam_x;
            related_y = pixel_ydisloc[p] - cam_y;
            related_z = pixel_zdisloc[p] - cam_z;
            pixel_absd[p] = sqrt(related_x*related_x+
                                related_y*related_y+
                                related_z*related_z);
    }
}

void Riproduci_Situazione (char i)
{
    int fh;
    //unsigned conta;
    if (i >= 'a' && i <= 'z') {
        i -= 'a' - 'A';
    }

    sprintf (t, "CRYXTELS.%cIT", i);

    fh = open(t, O_RDONLY);
    if (fh != -1) {
        short tmp_pixels;
        read (fh, &tmp_pixels, sizeof(short));
        if (tmp_pixels == 0) {
            cerr << "Failed to read game from \"" << t << "\": Invalid situation file." << endl;
            close(fh);
            return;
        }
        fade ();
        pixels = tmp_pixels;
        read (fh, &pixel_support[0], sizeof(double)*pixels);
        read (fh, &pixel_xdisloc[0], sizeof(double)*pixels);
        read (fh, &pixel_ydisloc[0], sizeof(double)*pixels);
        read (fh, &pixel_zdisloc[0], sizeof(double)*pixels);
        read (fh, &objects, sizeof(short));
        _objects = objects;
        read (fh, &objecttype[0], sizeof(short)*objects);
        read (fh, &relative_x[0], sizeof(double)*objects);
        read (fh, &relative_y[0], sizeof(double)*objects);
        read (fh, &relative_z[0], sizeof(double)*objects);
        read (fh, &absolute_x[0], sizeof(double)*objects);
        read (fh, &absolute_y[0], sizeof(double)*objects);
        read (fh, &absolute_z[0], sizeof(double)*objects);
        read (fh, &object_location[0], sizeof(short)*objects);
        read (fh, &cam_x, sizeof(double));
        read (fh, &cam_y, sizeof(double));
        read (fh, &cam_z, sizeof(double));
        read (fh, &alfa, sizeof(short));
        read (fh, &beta, sizeof(short));
        read (fh, &nav_a, sizeof(short));
        read (fh, &nav_b, sizeof(short));
        read (fh, &taking, 1);
        read (fh, &carry_type, sizeof(short));
        read (fh, &trackframe, sizeof(double));
        read (fh, &reset_trackframe, 1);
        read (fh, &tracking, sizeof(double));
        read (fh, &req_end_extra, 1);
        read (fh, &alfad, sizeof(short));
        read (fh, &betad, sizeof(short));
        read (fh, &pix, sizeof(short));
        read (fh, &alfa90, sizeof(short));
        read (fh, &beta90, sizeof(short));
        read (fh, &fid, 1);
        read (fh, &lead, 1);
        read (fh, &orig, 1);
        read (fh, &comera_m, 1);
        read (fh, &spd_x, sizeof(double));
        read (fh, &spd_y, sizeof(double));
        read (fh, &spd_z, sizeof(double));
        read (fh, &spd, sizeof(double));
        read (fh, &extra, 1);
        read (fh, &rel_x, sizeof(double));
        read (fh, &rel_y, sizeof(double));
        read (fh, &rel_z, sizeof(double));
        read (fh, &obj, sizeof(short));
        read (fh, &m, 1);
        read (fh, &echo, 1);
        read (fh, &carried_pixel, sizeof(short));
        read (fh, &disl, sizeof(double));
        read (fh, &cursore, sizeof(short));
        read (fh, &explode_count, 1);
        read (fh, &ctrlkeys[0], 1);
        read (fh, &pixel_rot[0], pixels);
        read (fh, &pixeltype[0], sizeof(short)*pixels);
        read (fh, &repeat, 1);
        read (fh, &source, 1);
        read (fh, &quality, 1);
        close (fh);
        rot ();
        dists ();
        dock_effects ();
        //sta_suonando = -1;
        //pixel_sonante = -1;
        //globalvocfile[0] = '.';
        mx = beta * 5; my = alfa * 5;
        r_x = rel_x; r_y = rel_y; r_z = rel_z;
        SDL_LockSurface(p_surface);
        pclear (static_cast<unsigned char*>(p_surface->pixels), 0);
        SDL_UnlockSurface(p_surface);
        //for (c = 0; c<objects; c++)
                //if (absolute_y[c]>=10E10) absolute_y[c]=0;
        justloaded = 1;
        cout << "Game [" << i << "] successfully loaded." << endl;
    } else {
        auto err = errno;
        cerr << "Failed to read game from \"" << t << "\": " << strerror(err) << endl;
    }
}

void snapshot()
{
    SDL_SaveBMP(p_surface, "SNAP.bmp");
}

void ispd ()
{
    int acount;

    if (trackframe&&!extra) return;
    if (!extra&&trackframe<23) {
        //spd = 2*spd + 1;
        spd = 1.5*spd + 0.5;
        if (spd>300) spd = 300;
    }
    else {
        _x = rel_x;
        _z = rel_z;
        acount = (double)(SDL_GetTicks()%(FRAMES_PER_SECOND/4)) - 4;
        // Sound off
        //if (acount<0&&!sbp_stat) play (PASSO);
        rel_y += (double)acount / 5;
        rel_x -= /* 4 */ 2 * tsin[beta] * tcos[alfa];
        rel_z += /* 4 */ 2 * tcos[beta] * tcos[alfa];
        if (docksite_h[pixeltype[pix]]>=0) {
            if (rel_x>docksite_w[pixeltype[pix]]-docksite_x[pixeltype[pix]]) rel_x = docksite_w[pixeltype[pix]]-docksite_x[pixeltype[pix]];
            if (rel_x<-docksite_x[pixeltype[pix]]-docksite_w[pixeltype[pix]]) rel_x = -docksite_x[pixeltype[pix]]-docksite_w[pixeltype[pix]];
            if (rel_z>docksite_h[pixeltype[pix]]-docksite_z[pixeltype[pix]]) rel_z = docksite_h[pixeltype[pix]]-docksite_z[pixeltype[pix]];
            if (rel_z<-docksite_z[pixeltype[pix]]-docksite_h[pixeltype[pix]]) rel_z = -docksite_z[pixeltype[pix]]-docksite_h[pixeltype[pix]];
        }
        else {
            rel_x += docksite_x[pixeltype[pix]];
            rel_z += docksite_z[pixeltype[pix]];
            if (sqrt(rel_x*rel_x+rel_z*rel_z)>docksite_w[pixeltype[pix]]) {
                rel_x = _x;
                rel_z = _z;
            }
            else {
                rel_x -= docksite_x[pixeltype[pix]];
                rel_z -= docksite_z[pixeltype[pix]];
            }
        }
    }
}

void dspd ()
{
        if (trackframe&&!extra) return;
        if (!extra&&trackframe<23)
                spd = 0;
        else {
                // Sound off
                //if (!sbp_stat) play (PASSO);
                _x = rel_x;
                _z = rel_z;
                rel_x += 4 * tsin[beta] * tcos[alfa];
                rel_z -= 4 * tcos[beta] * tcos[alfa];
                if (docksite_h[pixeltype[pix]]>=0) {
                        if (rel_x>docksite_w[pixeltype[pix]]-docksite_x[pixeltype[pix]]) rel_x = docksite_w[pixeltype[pix]]-docksite_x[pixeltype[pix]];
                        if (rel_x<-docksite_x[pixeltype[pix]]-docksite_w[pixeltype[pix]]) rel_x = -docksite_x[pixeltype[pix]]-docksite_w[pixeltype[pix]];
                        if (rel_z>docksite_h[pixeltype[pix]]-docksite_z[pixeltype[pix]]) rel_z = docksite_h[pixeltype[pix]]-docksite_z[pixeltype[pix]];
                        if (rel_z<-docksite_z[pixeltype[pix]]-docksite_h[pixeltype[pix]]) rel_z = -docksite_z[pixeltype[pix]]-docksite_h[pixeltype[pix]];
                }
                else {
                        rel_x += docksite_x[pixeltype[pix]];
                        rel_z += docksite_z[pixeltype[pix]];
                        if (sqrt(rel_x*rel_x+rel_z*rel_z)>docksite_w[pixeltype[pix]]) {
                                rel_x = _x;
                                rel_z = _z;
                        }
                        else {
                                rel_x -= docksite_x[pixeltype[pix]];
                                rel_z -= docksite_z[pixeltype[pix]];
                        }
                }
        }
}

void attracco ()
{
    play (ATTRACCO);
    trackframe = 0;
    tracking = 1;
    alfad = 0;
    betad = 0;
}

void undock ()
{
    if (!extra) {
        if (trackframe==23) {
            spd_x = pixel_xdisloc[pix] - prevpixx;
            spd_z = pixel_zdisloc[pix] - prevpixz;
            reset_trackframe = 1;
            play (DISTACCO); subs = 0;
        }
        else {
            if (pixel_absd[pix] < 500 + 1000*pixel_rot[pix]
                    &&pixel_ydisloc[pix] + docksite_y[pixeltype[pix]] - 16 > cam_y
                    &&!trackframe&&!explode_count)
                attracco ();
            }
    }
}

void extra_stop_extra ()
{
    if (extra) {
        d = sqrt(rel_x*rel_x+(rel_z+10)*(rel_z+10));
        if (d<25) {
            req_end_extra = 1;
            type_mode = false;
            play (FLY_ON);
        }
    }
    else {
        if (alfa==0&&trackframe==23&&req_end_extra==0) {
            if (!moving_last_object) {
                play (FLY_OFF);
                rel_x = 0;
                rel_y = 7;
                rel_z = 0;
                extra = 1;
                dock_effects ();
            }
        }
    }
}

void find_alfabeta()
{
    kk = 1E9;
    for (c=0; c<360; c++) {
        x2 = cam_x - 100 * tsin[c] * tcos[alfa];
        z2 = cam_z + 100 * tcos[c] * tcos[alfa];
        y2 = cam_y + 100 * tsin[alfa];
        ox = rx - x2; oy = ry - y2; oz = rz - z2;
        ox = ox*ox+oy*oy+oz*oz;
        if (ox<kk) {
            kk = ox;
            beta90 = c;
        }
    }
    kk = 1E9;
    for (c=0; c<360; c++) {
        x2 = cam_x - 100 * tsin[beta90] * tcos[c];
        z2 = cam_z + 100 * tcos[beta90] * tcos[c];
        y2 = cam_y + 100 * tsin[c];
        ox = rx - x2; oy = ry - y2; oz = rz - z2;
        ox = ox*ox+oy*oy+oz*oz;
        if (ox<kk) {
            kk = ox;
            alfa90 = c;
        }
    }
    play (FID);
    subs = 0;
    comera_m = m;
    m = 0;
}

void fid_on ()
{
        if (extra||trackframe||fid||lead||orig) return;
        rx = cam_x + 100 * tsin[beta] * tcos[alfa];
        rz = cam_z - 100 * tcos[beta] * tcos[alfa];
        ry = cam_y - 100 * tsin[alfa];
        find_alfabeta ();
        fid = 1;
}

void lead_on ()
{
    if (extra||trackframe||fid||lead||orig) return;
    rx = cam_x + spd_x;
    ry = cam_y + spd_y;
    rz = cam_z + spd_z;
    find_alfabeta ();
    lead = 1;
}

void orig_on ()
{
    if (extra||trackframe||fid||lead||orig) return;
    rx = cam_x / 1.001;
    ry = cam_y / 1.001;
    rz = cam_z / 1.001;
    find_alfabeta ();
    orig = 1;
}

void dock_effects ()
{
    // Effetti dell'attracco.

    if (trackframe) {
        spd = 0;
        ox = pixel_xdisloc[pix] + docksite_x[pixeltype[pix]] - cam_x;
        oz = pixel_zdisloc[pix] + docksite_z[pixeltype[pix]] - cam_z;
        oy = pixel_ydisloc[pix] + docksite_y[pixeltype[pix]] - 16 - cam_y;
        if (oy<0&&!extra) cam_y = pixel_ydisloc[pix] + docksite_y[pixeltype[pix]] - 16;
        if (trackframe>12) tracking = 0.5;
        if (trackframe>21) tracking = 0.2;
        if (trackframe>22) tracking = 0.1;
        if (trackframe>=23) {
            spd_y = 0; // Perch altrimenti questa rimane.
            tracking = 0;
            trackframe = 23;
        }
        cam_x += ox / (24-trackframe);
        cam_y += oy / (24-trackframe);
        cam_z += oz / (24-trackframe);
        dsx = cam_x; dsy = cam_y; dsz = cam_z;
        if (fabs(oy)<152*fabs(tsin[alfa])&&!extra&&alfa) {
            if (alfa>179) alfa-=360;
            alfa = (double) alfa / 1.1;
            if (alfa<0) alfa+=360;
        }
        if (extra) {
            if (!req_end_extra)
                dsy -= 36;
            else
                dsy -= 24 / (double)req_end_extra;
        }
    }
}

void scoppia (int nr_ogg, double potenza, int var)
{
        for (a=0; a<180; a+=18)
                for (b=36; b<180; b+=18) {
                        oz = potenza * tcos[b];
                        z2 = potenza * tsin[b];
                        ox = z2 * tcos[a];
                        oy = z2 * tsin[a];
                        Line3D (absolute_x[nr_ogg]-ox, absolute_y[nr_ogg]-oz, absolute_z[nr_ogg]+oy,
                                absolute_x[nr_ogg]+ox, absolute_y[nr_ogg]+oz, absolute_z[nr_ogg]-oy);
                }

        int bk = carry_type;
        preleva_oggetto (nr_ogg);
        carry_type = bk;

        if (!(SDL_GetTicks()%(random(var)+1))) play (BOM);
        subs = 0;
}

void chiudi_filedriver ()
{
/* No audio
    globalvocfile[0] = '.';
    if (recfile>-1) {
        audio_stop ();
        write (recfile, "\0", 1);
        close (recfile);
        recfile = -1;
    }
    file_driver_off ();
    dsp_driver_on ();
*/
}

void alfin (char arc)
{
        if (arc) fade ();
        //dsp_driver_off (); // unused right now
        if (recfile>-1) {
                //audio_stop (); // unused right now
                write (recfile, "\0",1);
                close (recfile);
        }
        //file_driver_off ();
        _80_25_C ();
        ctrlkeys[0] = ctk;
}

void preleva_oggetto (int nr_ogg)
{
    if (moving_last_object) return;

    if (globalvocfile[0]=='.'&&extra) play (PRENDERE);

    carry_type = objecttype[nr_ogg];
    if (extra&&object_location[nr_ogg]>-1&&carry_type==1)
        pixel_rot[pix]--;

    int o;
    for (o=nr_ogg; o<_objects; o++) {
        objecttype[o] = objecttype[o+1];
        absolute_x[o] = absolute_x[o+1];
        absolute_y[o] = absolute_y[o+1];
        absolute_z[o] = absolute_z[o+1];
        relative_x[o] = relative_x[o+1];
        relative_y[o] = relative_y[o+1];
        relative_z[o] = relative_z[o+1];
        object_location[o] = object_location[o+1];
    }

    _objects--;
}

void Oggetti_sul_Pixel (char oblige)
{
    int o;

    if (pixel_absd[nopix]>4000&&!oblige) return;
//    int fdstatus = 0;
    for (o=0; o<_objects; o++) {
                if (object_location[o]==nopix) {
                        ox = relative_x[o] + pixel_xdisloc[nopix];
                        oy = relative_y[o] + pixel_ydisloc[nopix];
                        oz = relative_z[o] + pixel_zdisloc[nopix];
                        oy -= object_elevation[objecttype[o]];
                        if (nopix==pix||oblige) {
                                if (explode_count) explode = explode_count;
                                if (objecttype[o]==2||objecttype[o]==0) {
                                        for (a=o+1; a<_objects; a++) {
                                                if (object_location[a]==nopix) {
                                                        _oy = relative_y[o] - relative_y[a];
                                                        if (_oy>=0) {
                                                                _ox = relative_x[o] - relative_x[a];
                                                                _oz = relative_z[o] - relative_z[a];
                                                                if (sqrt(_ox*_ox+_oy*_oy+_oz*_oz)<object_elevation[objecttype[a]]+5) {
                                                                        if (objecttype[o]==2) {
                                                                                /* Audio object type, I suppose. Off you go.
                                                                                if (objecttype[a] == sta_suonando) vicini = 1;
                                                                                if (!fd_status&&!moving_last_object) {
                                                                                        if (absolute_y[a]<10E10) {
                                                                                                if (!repeat)
                                                                                                        absolute_y[a] = 10E10;
                                                                                                else
                                                                                                        absolute_y[a] = 11E11;
                                                                                                ptr = 9*(objecttype[a]+FRONTIER_M3);
                                                                                                if (subsignal[ptr]) {
                                                                                                        if (!memcmp(&subsignal[ptr], "VISORE", 6)) {     goto nopoint; } // Do nothing.
                                                                                                        if (!memcmp(&subsignal[ptr], "TESTO-", 6)) {     goto nopoint; } // Do nothing.
                                                                                                        if (!memcmp(&subsignal[ptr], "BOMBA" , 5)) {     goto nopoint; } // Do nothing.
                                                                                                        if (!memcmp(&subsignal[ptr], "SORG-D", 6)) { source = 0;          goto nopoint; }
                                                                                                        if (!memcmp(&subsignal[ptr], "SORG-C", 6)) { source = 1;          goto nopoint; }
                                                                                                        if (!memcmp(&subsignal[ptr], "SORG-M", 6)) { source = 2;          goto nopoint; }
                                                                                                        if (!memcmp(&subsignal[ptr], "SORG-L", 6)) { source = 3;          goto nopoint; }
                                                                                                        if (!memcmp(&subsignal[ptr], "N-TAPE", 6)) { quality = 0;         goto nopoint; }
                                                                                                        if (!memcmp(&subsignal[ptr], "Q-TAPE", 6)) { quality = 1;         goto nopoint; }
                                                                                                        if (!memcmp(&subsignal[ptr], "C-DISC", 6)) { quality = 2;         goto nopoint; }
                                                                                                        if (!memcmp(&subsignal[ptr], "REPEAT", 6)) { repeat = 1 - repeat; goto nopoint; }
                                                                                                        if (recfile==-1) {
                                                                                                                memcpy (globalvocfile, &subsignal[ptr], 9);
                                                                                                                strcat (globalvocfile, ".voc");
                                                                                                                dsp_driver_off ();
                                                                                                                file_driver_on ();
                                                                                                                filtraggio (record_filt[quality]);
                                                                                                                if (audio_play (globalvocfile) == -1) {
                                                                                                                        sorgente (source);
                                                                                                                        recfile = _creat (globalvocfile, 0);
                                                                                                                        if (recfile>-1) rec (recfile, record_frq[quality]);
                                                                                                                }
                                                                                                                sta_suonando = objecttype[a];
                                                                                                                pixel_sonante = nopix;
                                                                                                        }
                                                                                                        vicini = 1;
                                                                                                        nopoint:
                                                                                                }
                                                                                        }
                                                                                }
                                                                                */
                                                                        }
                                                                        else {
                                                                                if (!moving_last_object) {
                                                                                        if (_objects<500&&absolute_y[a]<10E10) {
                                                                                                absolute_y[_objects] = 10E10;
                                                                                                objecttype[_objects] = objecttype[a];
                                                                                                relative_x[_objects] = relative_x[a];
                                                                                                relative_y[_objects] = relative_y[a] - object_elevation[a];
                                                                                                relative_z[_objects] = relative_z[a];
                                                                                                object_location[_objects] = nopix;
                                                                                                if (objecttype[_objects]==1) pixel_rot[nopix]++;
                                                                                                _objects++;
                                                                                        }
                                                                                        absolute_y[a] = 10E10;
                                                                                }
                                                                        }
                                                                }
                                                        }
                                                }
                                        }
                                }
                        }
            if (!oblige) Object (objecttype[o]);
            explode = 0;
        }
    }
}

void lascia_cadere ()
{
    //int a;

    if (trackframe&&!extra) return;
    if (moving_last_object) return;

    objecttype[_objects] = carry_type;

    if (extra) {
        relative_x[_objects] = cox - prevpixx;
        relative_z[_objects] = coz - prevpixz;
        relative_y[_objects] = coy - pixel_ydisloc[pix] + object_elevation[carry_type];
        _x = rel_x;
        _z = rel_z;
        rel_x -= 9 * tsin[beta] * tcos[alfa];
        rel_z += 9 * tcos[beta] * tcos[alfa];
        if (docksite_h[pixeltype[pix]]>=0) {
            if (rel_x>docksite_w[pixeltype[pix]]-docksite_x[pixeltype[pix]]) rel_x = docksite_w[pixeltype[pix]]-docksite_x[pixeltype[pix]];
            if (rel_x<-docksite_x[pixeltype[pix]]-docksite_w[pixeltype[pix]]) rel_x = -docksite_x[pixeltype[pix]]-docksite_w[pixeltype[pix]];
            if (rel_z>docksite_h[pixeltype[pix]]-docksite_z[pixeltype[pix]]) rel_z = docksite_h[pixeltype[pix]]-docksite_z[pixeltype[pix]];
            if (rel_z<-docksite_z[pixeltype[pix]]-docksite_h[pixeltype[pix]]) rel_z = -docksite_z[pixeltype[pix]]-docksite_h[pixeltype[pix]];
        }
        else {
            if (sqrt(rel_x*rel_x+rel_z*rel_z)>docksite_w[pixeltype[pix]]) {
                rel_x = _x;
                rel_z = _z;
            }
        }
        cam_xt = dsx + rel_x;
        cam_yt = dsy + rel_y;
        cam_zt = dsz + rel_z;
        lastblock_real_x = 1E9;
        CollyZone (pix, 1);
        cfx = rel_x;
        cfz = rel_z;
        if ((ctrlkeys[0]&3)&&lastblock_real_x<1E9) {
            cfx = lastblock_real_x - pixel_xdisloc[pix];
            cfz = lastblock_real_z - pixel_zdisloc[pix];
        }
        else {
            cfx += docksite_x[pixeltype[pix]];
            cfz += docksite_z[pixeltype[pix]];
        }
        cfy = rel_y + disl + docksite_y[pixeltype[pix]] - 7;
        object_location[_objects] = pix;
        if (carry_type==1) {
            pixel_support[pix] = atan (pixel_zdisloc[pix] / pixel_xdisloc[pix]);
            pixel_rot[pix]++;
        }
        rel_x = _x;
        rel_z = _z;
        disl = 0;
        moving_last_object = 1;
    }
    else {
        object_location[_objects] = -1;
        absolute_y[_objects] = cam_y + 9 * tsin[alfa];
        absolute_x[_objects] = cam_x - 9 * tsin[beta] * tcos[alfa];
        absolute_z[_objects] = cam_z + 9 * tcos[beta] * tcos[alfa];
        relative_y[_objects] = spd_y;
        relative_x[_objects] = spd_x;
        relative_z[_objects] = spd_z;
        if (ctrlkeys[0]&64) {
            relative_y[_objects] += 5 * tsin[alfa];
            relative_x[_objects] -= 5 * tsin[beta] * tcos[alfa];
            relative_z[_objects] += 5 * tcos[beta] * tcos[alfa];
        }
        /* Audio off
        if (subsignal[9*(carry_type+FRONTIER_M3)]) {
            if (carry_type==sta_suonando&&fd_status) break; //goto noeli;
            strcpy (t, &subsignal[9*(carry_type+FRONTIER_M3)]);
            strcat (t, ".voc");
            a = open (t, 0);
            if (a>-1) {
                close (a);
                remove (t);
            }
            //noeli:
        }
        */
    }

    carry_type = -1;
    _objects++;
}

void n (double sx, double sy, double sz, double fx, double fy, double fz)
{
        z2 = sz * tcos[nav_a] + sy * tsin[nav_a];
        sy = sy * tcos[nav_a] - sz * tsin[nav_a];
        rx = sx * tcos[nav_b] + z2 * tsin[nav_b];
        sz = z2 * tcos[nav_b] - sx * tsin[nav_b];
        sx = rx;

        z2 = fz * tcos[nav_a] + fy * tsin[nav_a];
        fy = fy * tcos[nav_a] - fz * tsin[nav_a];
        rx = fx * tcos[nav_b] + z2 * tsin[nav_b];
        fz = z2 * tcos[nav_b] - fx * tsin[nav_b];
        fx = rx;

        sx += nav_x; sy += nav_y; sz += nav_z;
        fx += nav_x; fy += nav_y; fz += nav_z;

        Line3D (sx, sy, sz, fx, fy, fz);
}

void nTxt (const char *testo, double x, double y, double z, double sx, double sy)
{
        int c=0;
        char transit[2] = {0, 0};
        double ntx, nty, ntz;

        z2 = z * tcos[nav_a] + y * tsin[nav_a];
        ntx = x * tcos[nav_b] + z2 * tsin[nav_b];
        nty = y * tcos[nav_a] - z * tsin[nav_a];
        ntz = z2 * tcos[nav_b] - x * tsin[nav_b];

        x += sx * mediumwidth;

        z2 = z * tcos[nav_a] + y * tsin[nav_a];
        double avan_x = x * tcos[nav_b] + z2 * tsin[nav_b];
        double avan_y = y * tcos[nav_a] - z * tsin[nav_a];
        double avan_z = z2 * tcos[nav_b] - x * tsin[nav_b];

        avan_x -= ntx;
        avan_y -= nty;
        avan_z -= ntz;

        while (testo[c]) {
                transit[0] = testo[c];
                Txt (transit, ntx, nty, ntz, sx, sy, nav_a, nav_b);
                ntx += avan_x; nty += avan_y; ntz += avan_z;
                c++;
        }
}

void nrect (double x, double y, double z, double l, double h)
{
        n (x-l, y-h, z, x+l, y-h, z);
        n (x+l, y-h, z, x+l, y+h, z);
        n (x+l, y+h, z, x-l, y+h, z);
        n (x-l, y+h, z, x-l, y-h, z);
}

void console_key (const char *serigraph, double x, char cod, char input, char cond_attu, char cond_prec)
{
        if (extra) {
                d = sqrt(rel_x*rel_x + (rel_z+10)*(rel_z+10));
                if (d>25) {
                        input = 1;
                        cod = 0;
                }
        }

        if (input!=cod) {
                n (x, 3.5, 12.01, x+1, 3.5, 12.01);
                n (x, 3.5, 12.01, x, 4, 12.01);
                n (x+1, 3.5, 12.01, x+1, 4, 12.01);
                n (x, 3.5, 12.01, x, 4, 12.2);
                n (x+1, 3.5, 12.01, x+1, 4, 12.2);
                n (x, 4, 12.2, x+1, 4, 12.2);
                n (x, 4, 12.01, x, 4, 12.2);
                n (x+1, 4, 12.01, x+1, 4, 12.2);
                nTxt (serigraph, x+0.2, 3.7, 12.01, 0.05, 0.05);
        }
        else {
                nrect (x+0.5, 3.75, 12.31, 0.5, 0.25);
                //n (x, 4, 12.31, x+1, 4, 12.31);
                //n (x, 3.5, 12.31, x+1, 3.5, 12.31);
                //n (x, 3.5, 12.31, x, 4, 12.31);
                //n (x+1, 3.5, 12.31, x+1, 4, 12.31);
                n (x, 3.5, 12.31, x, 4, 12.5);
                n (x+1, 3.5, 12.31, x+1, 4, 12.5);
                n (x, 4, 12.5, x+1, 4, 12.5);
                n (x, 4, 12.31, x, 4, 12.5);
                n (x+1, 4, 12.31, x+1, 4, 12.5);
                nTxt (serigraph, x+0.2, 3.7, 12.31, 0.05, 0.05);
                if (cond_attu!=cond_prec) {
                        if (extra&&globalvocfile[0]!='.') return;
                        //if (!sbp_stat) play (TASTO);
                        subs = 0;
                }
        }
}

void collyblock (double cx, double cy, double cz,
                 double hx, double hy, double hz,
                 char blocktype)
{
    if (!dropping) {
        r_y = rel_y;
        cx += ox; cy += oy; cz += oz;
        if (cam_xt>=cx-hx&&cam_xt<=cx+hx) {
            if (cam_zt>=cz-hz&&cam_zt<=cz+hz) {
                if ((blocktype==BLOCCO_ELEVATO||blocktype==SOLID_BOX)&&(cam_yt+45>cy+hy))
                    return;
                onblock = 1;
                if (blocktype==BLOCCO_PROIBITO) {
                    rel_x = r_x;
                    rel_y = r_y;
                    rel_z = r_z;
                    return;
                }
                if (cy-hy>=max_y) return;
                max_y = cy-hy;
                if (cam_yt+45!=cy-hy) {
                    rel_y = (cy-hy)-45-dsy;
                    cam_yt = dsy + rel_y;
                }
                disl = rel_y-r_y;
                rel_y = r_y;
                if (fabs(disl)>LIMITE_DISLIVELLI) {
                    rel_x = r_x;
                    rel_z = r_z;
                    disl = 0;
                }
                else {
                    lastblock_real_x = cx;
                    lastblock_real_z = cz;
                    }
                }
            }
        }
        else {
            if (rel_x>=cx-hx&&rel_x<=cx+hx) {
                if (rel_z>=cz-hz&&rel_z<=cz+hz) {
                    if (blocktype==BLOCCO_ELEVATO || blocktype == SOLID_BOX) return;
                    if (blocktype==BLOCCO_PROIBITO)
                        onblock = 2;
                    else
                        onblock = 1;
                    if (cy-hy>=max_y) return;
                    max_y = cy-hy;
                    if (rel_y!=cy-hy-dsy)
                        rel_y = cy-hy-dsy;
            }
        }
    }
}

void CollyZone (int nopix, char objectblocks)
{
        int o;
        long jjj;
        int iii, nr_elem = 0;

        max_y = 10E9;
        onblock = 0;
        disl = 0;

        ox = pixel_xdisloc[nopix];
        oy = pixel_ydisloc[nopix];
        oz = pixel_zdisloc[nopix];

ricerca:if (loaded_pixeltypes) {
                for (iii = 0; iii<loaded_pixeltypes; iii++) {
                        if (pixeltype[nopix]==pixeltype_type[iii])
                                goto trovato;
                }
        }
        LoadPtyp (pixeltype[nopix]);
        goto ricerca;
trovato:while (nr_elem<pixeltype_elements[iii]) {
                jjj = ELEMS * iii + nr_elem;
                if (pixel_elem_t[jjj]==BLOCCO_COLLISIONE
                  ||pixel_elem_t[jjj]==BLOCCO_PROIBITO
                  ||pixel_elem_t[jjj]==BLOCCO_ELEVATO
                  ||pixel_elem_t[jjj]==SOLID_BOX ) {
                        collyblock (pixel_elem_x[jjj],
                                    pixel_elem_y[jjj],
                                    pixel_elem_z[jjj],
                                    pixel_elem_1[jjj],
                                    pixel_elem_2[jjj],
                                    pixel_elem_3[jjj],
                                    pixel_elem_t[jjj]);
                }
                nr_elem ++;
        }

        if (objectblocks||(echo&4)) {
                for (o=0; o<_objects; o++) {
                        if (object_location[o]==nopix) {
                                ox = relative_x[o] + pixel_xdisloc[nopix];
                                oy = relative_y[o] + pixel_ydisloc[nopix];
                                oz = relative_z[o] + pixel_zdisloc[nopix];
                                oy -= object_elevation[objecttype[o]];
                                if (objecttype[o]>2) {
                                        c = objecttype[o] + FRONTIER_M3;
                                        collyblock (0, object_collyblockshifting[c-FRONTIER_M3], 0, docksite_x[c], docksite_y[c], docksite_z[c], BLOCCO_COLLISIONE);
                                }
                                else
                                        collyblock (0, 0, 0, 6, 0.01, 6, BLOCCO_COLLISIONE);
                        }
                }
        }

        if (!onblock) {
                if (fabs(7 - rel_y) > LIMITE_DISLIVELLI) {
                        rel_x = r_x;
                        rel_z = r_z;
                }
                else
                        disl = 7 - rel_y;
        }

        if (taking) {
                kk = 40;
                for (o=_objects-1; o>=0; o--) {
                        // ctrlkeys[0]&64 is Caps Lock
                        if (object_location[o]==nopix&&(pixelmass[objecttype[o]+FRONTIER_M3]<100||(ctrlkeys[0]&64))) {
                                _x = (relative_x[o] + pixel_xdisloc[nopix]) - (cam_xt - 10 * tsin[beta] * tcos[alfa]);
                                _y = (relative_y[o] + pixel_ydisloc[nopix] - object_elevation[objecttype[o]]) - (cam_yt + 10 * tsin[alfa]);
                                _z = (relative_z[o] + pixel_zdisloc[nopix]) - (cam_zt + 10 * tcos[beta] * tcos[alfa]);
                                _y /= 3;
                                d = sqrt (_x*_x+_y*_y+_z*_z);
                                if (d<kk) {
                                        obj = o;
                                        kk = d;
                                }
                        }
                }
        }
}

/// Use share_x and share_y to make a simple thick plot
void aux_plot()
{
    auto di = share_x+riga[share_y];
    SDL_LockSurface(p_surface);
//    asm {
//    les si, dword ptr adapted
    auto si = static_cast<unsigned char*>(p_surface->pixels);
//    add si, di
    si += di;
//    cmp byte ptr es:[si], 32
//    jnb a_tz
    if (si[0] < 32) {
        si[0] += 8;
        si[1] += 4;
        si[-1] += 4;
        si[WIDTH] += 4;
        si[-WIDTH] += 4;
//    add byte ptr es:[si], 8
//    add byte ptr es:[si+1], 4
//    add byte ptr es:[si-1], 4
//    add byte ptr es:[si+320], 4
//    add byte ptr es:[si-320], 4
//    }
    }
    SDL_UnlockSurface(p_surface);
}

void Pixel (int typ)
{
    double p0, p1, p2, p3, p4, p5, p6, crx=0, cry=0, crz=0;
    double first_x, first_y, first_z;

    int iii, pid = 3, nr_elem = 0;
    long jjj, bkuneg = uneg;
    unsigned vptr;

    if (typ>FRONTIER_M1) goto ricerca;

    ox = pixel_xdisloc[nopix];
    oy = pixel_ydisloc[nopix];
    oz = pixel_zdisloc[nopix];

    if (subsignal[9*typ]&&pixel_absd[nopix]>512) {
        p0 = pixel_absd[nopix] * 0.01; if (p0<628) p0 = 628;
        c = pixel_absd[nopix] / 700; if (c>45) c = 45; if (c<18) c = 18;
        for (a=0; a<360; a+=c) {
            for (b=c; b<180; b+=c) {
                if (C32(ox+p0*tsin[b]*tcos[a], oy+p0*tcos[b], oz-p0*tsin[a]*tsin[b])) {
//                     _DI = share_x+riga[share_y];
                    SDL_LockSurface(p_surface);
                    auto di = share_x + riga[share_y];
//               asm {
//                     les si, dword ptr adapted
                    auto si = static_cast<unsigned char*>(p_surface->pixels);
//                     add si, di
                    si += di;
//                     cmp byte ptr es:[si], 32
//                     jnb e_tz
                    if (*si < 32) {
                        si[0] += 7;
                        si[1] += 5; si[-1] += 5;
                        si[WIDTH] += 5;  si[-WIDTH] += 5;
                        si[WIDTH+1] += 3; si[-WIDTH-1] += 3;
                        si[-WIDTH+1] += 3; si[WIDTH-1] += 3;
//                     add byte ptr es:[si], 7
//                     add byte ptr es:[si+1], 5
//                     add byte ptr es:[si-1], 5
//                     add byte ptr es:[si+320], 5
//                     add byte ptr es:[si-320], 5
//                     add byte ptr es:[si+321], 3
//                     add byte ptr es:[si-321], 3
//                     add byte ptr es:[si+319], 3
//                     add byte ptr es:[si-319], 3
//                  }
                    }
//                        e_tz:
                    SDL_UnlockSurface(p_surface);
                    }
                }
            }
        }

        if (pixel_absd[nopix]>pixelmass[typ]) {
            if (!C32(ox, oy, oz)) return;
            vptr = share_x+riga[share_y];
            SDL_LockSurface(p_surface);
//          _CL = pixelmass[typ] / 1000; if (_CL>32) _CL = 32;
            auto cl = pixelmass[typ] / 1000; if (cl > 32) cl = 32;
//          _CH = _CL / 2; _DL = _CL / 4;
            auto ch = cl / 2; auto dl = cl / 4;
//                asm {
//                        les si, dword ptr adapted
            auto si = static_cast<unsigned char*>(p_surface->pixels);
//                        add si, vptr
            si += vptr;
//                        cmp byte ptr es:[si], 32
//                        jnb tz_0
            if (si[0] < 32 ) {
                si[0] += cl;
                si[1] += ch; si[-1] += ch;
                si[2] += dl; si[-2] += dl;
                si[WIDTH-1] += dl; si[-WIDTH+1] += dl;
                si[WIDTH+1] += dl; si[-WIDTH-1] += dl;
                si[WIDTH] += ch; si[-WIDTH] += ch;
                si[WIDTH*2] += dl; si[-WIDTH*2] += dl;
//              add byte ptr es:[si], cl
//              add byte ptr es:[si+1], ch
//              add byte ptr es:[si-1], ch
//              add byte ptr es:[si+2], dl
//              add byte ptr es:[si-2], dl
//              add byte ptr es:[si+319], dl
//              add byte ptr es:[si-319], dl
//              add byte ptr es:[si+320], ch
//              add byte ptr es:[si-320], ch
//              add byte ptr es:[si+640], dl
//              add byte ptr es:[si-640], dl
//              add byte ptr es:[si+321], dl
//              add byte ptr es:[si-321], dl
//          }
            }
  //  tz_0:
        } else {
    ricerca:
            if (loaded_pixeltypes) {
                for (iii = 0; iii<loaded_pixeltypes; iii++) {
                    if (typ==pixeltype_type[iii])
                        goto trovato;
                }
            }
            Aggiornamento ();
            goto ricerca;
trovato:
            if (typ>FRONTIER_M1) {
                _x = ox - cam_x; _y = oy - cam_y; _z = oz - cam_z;
                d = sqrt(_x*_x+_y*_y+_z*_z);
                id = (int)(d / pixelmass[typ]);
                goto ogg_2;
            }
            else {
                id = (int) (pixel_absd[nopix]/CULLING);
                if (nopix==pix) explode = explode_count;
            }
            uneg += pixel_absd[nopix] / 100;
            if (uneg>1000) uneg = 1000;
ogg_2:
            if (id>3) id = 3;
            while (nr_elem<pixeltype_elements[iii]) {
                jjj = ELEMS * iii + nr_elem;
                if (pixel_elem_t[jjj]==DETTAGLIO) {
                    if (id==pid) {
                        uneg = bkuneg;
                        explode = 0;
                        return;
                    }
                    pid--;
                }
                p0 = pixel_elem_x[jjj];
                p1 = pixel_elem_y[jjj];
                p2 = pixel_elem_z[jjj];
                p3 = pixel_elem_1[jjj];
                p4 = pixel_elem_2[jjj];
                p5 = pixel_elem_3[jjj];
                p6 = pixel_elem_4[jjj];
                switch (pixel_elem_t[jjj]) {
                    case SPIRALE:
                        if (!p5) par0 (nr_elem, pixeltype[nopix]);
                        c = p5 * (id+1);
                        p3 *= id+1;
                        switch ((int)p4) {
                            case 0:
                                a = c; k1 = 0;
                                crx = c; cry = 0;
                                while (a<1080) {
                                    rel (p0+k1*tcos[(int)crx], p1+k1*tsin[(int)crx], p2,
                                            p0+k1*tcos[(int)cry], p1+k1*tsin[(int)cry], p2);
                                    cry = crx; crx += c;
                                    if (crx>359) crx-=360;
                                    k1 += p3;
                                    a += c;
                                }
                                break;
                            case 1:
                                a = c; k1 = 0;
                                crx = c; cry = 0;
                                while (a<1080) {
                                    rel (p0+k1*tcos[(int)crx], p1, p2+k1*tsin[(int)crx],
                                            p0+k1*tcos[(int)cry], p1, p2+k1*tsin[(int)cry]);
                                    cry = crx; crx += c;
                                    if (crx>359) crx-=360;
                                    k1 += p3;
                                    a += c;
                                }
                                break;
                            case 2:
                                a = c; k1 = 0;
                                crx = c; cry = 0;
                                while (a<1080) {
                                    rel (p0, p1+k1*tcos[(int)crx], p2+k1*tsin[(int)crx], p0,
                                            p1+k1*tcos[(int)cry], p2+k1*tsin[(int)cry]);
                                    cry = crx; crx += c;
                                    if (crx>359) crx-=360;
                                    k1 += p3;
                                    a += c;
                                }
                            }
                            break;
                        case PUNTO:
                            if (!id) xrel (p0, p1, p2);
                            break;
                        case LINEA:
                            rel (p0, p1, p2, p3, p4, p5);
                            break;
                        case RETTANGOLO:
                            rectrel (p0, p1, p2, p3, p4, p5);
                            break;
                        case SCATOLA:
                        case SOLID_BOX:
                            boxrel (p0, p1, p2, p3, p4, p5);
                            break;
                        case GRIGLIA:
                            p5 = (int) p5;
                            if (p5<=0) par0 (nr_elem, pixeltype[nopix]);
                            if (id) {
                                c = p5;
                                p5 /= id;
                                if (p5<1) p5 = 1;
                                p3 *= (float)c / p5;
                                p4 *= (float)c / p5;
                            }
                            _x = p3;
                            _y = p4;
                            switch ((int)p6) {
                                case 0:
                                    p0 -= (p5*_x) / 2;
                                    p1 -= (p5*_y) / 2;
                                    p6 = p1 + _y*p5; _z = p0;
                                    for (a=0; a<=p5; a++) {
                                        rel (p0, p1, p2, p0, p6, p2);
                                        p0 += _x;
                                    }
                                    p0 = _z; p6 = p0 + p5*_x;
                                    for (a=0; a<=p5; a++) {
                                        rel (p0, p1, p2, p6, p1, p2);
                                        p1 += _y;
                                    }
                                    break;
                                case 1:
                                    p0 -= (p5*_x) / 2;
                                    p2 -= (p5*_y) / 2;
                                    p6 = p2 + _y*p5; _z = p0;
                                    for (a=0; a<=p5; a++) {
                                        rel (p0, p1, p2, p0, p1, p6);
                                        p0 += _x;
                                    }
                                    p0 = _z; p6 = p0 + p5*_x;
                                    for (a=0; a<=p5; a++) {
                                        rel (p0, p1, p2, p6, p1, p2);
                                        p2 += _y;
                                    }
                                    break;
                                case 2:
                                    p2 -= (p5*_x) / 2;
                                    p1 -= (p5*_y) / 2;
                                    p6 = p1 + _y*p5; _z = p2;
                                    for (a=0; a<=p5; a++) {
                                        rel (p0, p1, p2, p0, p6, p2);
                                        p2 += _x;
                                    }
                                    p2 = _z; p6 = p2 + p5*_x;
                                    for (a=0; a<=p5; a++) {
                                        rel (p0, p1, p2, p0, p1, p6);
                                        p1 += _y;
                                    }
                                }
                                break;
                            case DISEGNO_ELLITTICO:
                                if (p6<=0) par0 (nr_elem, pixeltype[nopix]);
                                p6 *= id+1;
                                if (p6>90) p6 = 90;
                                if (p5==0) {
                                    for (c=0; c<360; c+=p6) {
                                        if (C32 (ox+p0+tcos[c]*p3, oy+p1+tsin[c]*p4, oz+p2)) {
                                            aux_plot();
                                        }
                                    }
                                    break;
                                }
                                if (p5==1) {
                                    for (c=0; c<360; c+=p6) {
                                        if (C32 (ox+p0+tcos[c]*p3, oy+p1, oz+p2+tsin[c]*p4)) {
                                            aux_plot();
                                        }
                                    }
                                    break;
                                }
                                if (p5==2) {
                                    for (c=0; c<360; c+=p6) {
                                        if (C32 (ox+p0, oy+p1+tsin[c]*p4, oz+p2+tcos[c]*p3)) {
                                            aux_plot();
                                        }
                                    }
                                    break;
                                }
                                break;
                            case ONDA:
                                if (p6<=0) par0 (nr_elem, pixeltype[nopix]);
                                p6 *= id+1;
                                if (p6>90) p6 = 90;
                                switch ((int)p5) {
                                    case 0:
                                        _x = p0 - p3*180 + p6*p3;
                                        _oy = p1; _ox = _x - p6*p3;
                                        for (c=p6; c<=360; c+=p6) {
                                            _y = p1 + tsin[c] * p4;
                                            rel (_x, _y, p2, _ox, _oy, p2);
                                            _oy = _y;
                                            _ox = _x;
                                            _x += p6*p3;
                                        }
                                        break;
                                    case 1:
                                        _z = p2 - p3*180 + p6*p3;
                                        _ox = p0; _oz = _z - p6*p3;
                                        for (c=p6; c<=360; c+=p6) {
                                            _x = tsin[c] * p4;
                                            rel (_x, p1, _z, _ox, p1, _oz);
                                            _ox = _x;
                                            _oz = _z;
                                            _z += p6*p3;
                                        }
                                        break;
                                    case 2:
                                        _z = p2 - p3*180 + p6*p3;
                                        _oy = p1; _oz = _z - p6*p3;
                                        for (c=p6; c<=360; c+=p6) {
                                            _y = p1 + tsin[c] * p4;
                                            rel (p0, _y, _z, p0, _oy, _oz);
                                            _oy = _y;
                                            _oz = _z;
                                            _z += p6*p3;
                                        }
                                    }
                                    break;
                                case COLONNA:
                                    if (p6<=0) par0 (nr_elem, pixeltype[nopix]);
                                    p6 *= id+1;
                                    if (p6>90) p6 = 90;
                                    _x = p0 + tcos[0]*p4;
                                    _z = p2 + tsin[0]*p4;
                                    _y = p1 - p5 / 2;
                                    crx = p0 + tcos[0]*p3;
                                    crz = p2 + tsin[0]*p3;
                                    cry = p1 + p5 / 2;
                                    for (a=p6; a<360; a+=p6) {
                                        _ox = _x;
                                        _oz = _z;
                                        first_x = crx;
                                        first_z = crz;
                                        _x = p0 + tcos[a]*p4;
                                        _z = p2 + tsin[a]*p4;
                                        crx = p0 + tcos[a]*p3;
                                        crz = p2 + tsin[a]*p3;
                                        rel (_x, _y, _z, crx, cry, crz);
                                        if (p4) rel (_x, _y, _z, _ox, _y, _oz);
                                        if (p3) rel (first_x, cry, first_z, crx, cry, crz);
                                    }
                                    _ox = _x;
                                    _oz = _z;
                                    first_x = crx;
                                    first_z = crz;
                                    _x = p0 + tcos[0]*p4;
                                    _z = p2 + tsin[0]*p4;
                                    crx = p0 + tcos[0]*p3;
                                    crz = p2 + tsin[0]*p3;
                                    rel (_x, _y, _z, crx, cry, crz);
                                    if (p4) rel (_x, _y, _z, _ox, _y, _oz);
                                    if (p3) rel (first_x, cry, first_z, crx, cry, crz);
                                    break;
                                case ASTERISCO:
                                    if (p4<=0) par0 (nr_elem, pixeltype[nopix]);
                                    p3 = -p3;
                                    p4 *= id+1;
                                    if (p4>90) p4 = 90;
                                    for (a=0; a<180; a+=p4)
                                        for (b=p4; b<180; b+=p4) {
                                            _oz = p3 * tcos[b];
                                            z2 = p3 * tsin[b];
                                            _ox = z2 * tcos[a];
                                            _oy = z2 * tsin[a];
                                            rel (p0-_ox, p1-_oz, p2+_oy, p0+_ox, p1+_oz, p2-_oy);
                                        }
                                    break;
                                case SFERA:
                                    if (p5<=0) par0 (nr_elem, pixeltype[nopix]);
                                    c = p5 * (id+1);
                                    if (c>90) c = 90;
                                    p3 = -p3;
                                    _ox = ox;
                                    _oy = oy;
                                    _oz = oz;
                                    crx = ox + p0;
                                    cry = oy + p1;
                                    crz = oz + p2;
                                    for (a=0; a<360; a+=c)
                                        for (b=c; b<180; b+=c) {
                                            oz = p3 * tcos[b] * p4;
                                            z2 = p3 * tsin[b];
                                            ox = z2 * tcos[a];
                                            oy = z2 * tsin[a];
                                            if (C32(ox+crx, oz+cry, crz-oy)) {
                                                aux_plot();
                                            }
                                        }
                                    ox = _ox;
                                    oy = _oy;
                                    oz = _oz;
                                    break;
                                case TESTO:
                                    sprintf (t, &pixel_elem_b[40*jjj], nopix);
                                    Txt (t, p0, p1, p2, p3, p4, p5, p6);
                                    break;
                                case ELLISSE:
                                    if (p6<=0) par0 (nr_elem, pixeltype[nopix]);
                                    p6 *= id+1;
                                    if (p6>90) p6 = 90;
                                    if (p5==0) {
                                        _x = p0+p3;
                                        _y = p1;
                                        for (c=p6; c<360; c+=p6) {
                                            crx = p0+tcos[c]*p3;
                                            cry = p1+tsin[c]*p4;
                                            rel (_x, _y, p2, crx, cry, p2);
                                            _x = crx;
                                            _y = cry;
                                        }
                                        rel (_x, _y, p2, p0+p3, p1, p2);
                                        break;
                                    }
                                    if (p5==1) {
                                        _x = p0+p3;
                                        _z = p2;
                                        for (c=p6; c<360; c+=p6) {
                                            crx = p0+tcos[c]*p3;
                                            crz = p2+tsin[c]*p4;
                                            rel (_x, p1, _z, crx, p1, crz);
                                            _x = crx;
                                            _z = crz;
                                        }
                                        rel (_x, p1, _z, p0+p3, p1, p2);
                                        break;
                                    }
                                    if (p5==2) {
                                        _y = p1+p3;
                                        _z = p2;
                                        for (c=p6; c<360; c+=p6) {
                                            cry = p1+tcos[c]*p3;
                                            crz = p2+tsin[c]*p4;
                                            rel (p0, _y, _z, p0, cry, crz);
                                            _y = cry;
                                            _z = crz;
                                        }
                                        rel (p0, _y, _z, p0, p1+p3, p2);
                                        break;
                                    }
                                    break;
                                case CIAMBELLA:
                                    if (p6<=0) par0 (nr_elem, pixeltype[nopix]);
                                    p6 *= id+1;
                                    if (p6>90) p6 = 90;
                                    _ox = ox;
                                    _oy = oy;
                                    _oz = oz;
                                    ox += p0;
                                    oy += p1;
                                    oz += p2;
                                    switch (int(p5)) {
                                        case 0:
                                            for (a=p6; a<360+p6; a+=p6) {
                                                first_x = (p4 - p3) * tcos[a];
                                                cry = first_x;
                                                first_y = 0;
                                                first_z = (p3-p4)*tsin[a];
                                                for (c=p6; c<360; c+=p6*2) {
                                                    _x = tcos[c]*p4 - p3;
                                                    _y = tsin[c]*p4;
                                                    crx = _x*tcos[a];
                                                    crz = - _x*tsin[a];
                                                    rel (crx, crz, _y, first_x, first_z, first_y);
                                                    first_x = crx; first_y = _y; first_z = crz;
                                                    rel (crx, crz, _y, _x*tcos[(int)(a-p6)], - _x*tsin[(int)(a-p6)], _y);
                                                }
                                                rel (crx, crz, _y, cry, (p3-p4) * tsin[a], 0);
                                            }
                                            break;
                                        case 1:
                                            for (a=p6; a<360+p6; a+=p6) {
                                                first_x = (p4 - p3) * tcos[a];
                                                cry = first_x;
                                                first_y = 0;
                                                first_z = (p3-p4)*tsin[a];
                                                for (c=p6; c<360; c+=p6*2) {
                                                    _x = tcos[c]*p4 - p3;
                                                    _y = tsin[c]*p4;
                                                    crx = _x*tcos[a];
                                                    crz = - _x*tsin[a];
                                                    rel (crx, _y, crz, first_x, first_y, first_z);
                                                    first_x = crx; first_y = _y; first_z = crz;
                                                    rel (crx, _y, crz, _x*tcos[(int)(a-p6)], _y, - _x*tsin[static_cast<int>(a-p6)]);
                                                }
                                                rel (crx, _y, crz, cry, 0, (p3-p4) * tsin[a]);
                                            }
                                            break;
                                        case 2:
                                            for (a=p6; a<360+p6; a+=p6) {
                                                first_x = (p4 - p3) * tcos[a];
                                                cry = first_x;
                                                first_y = 0;
                                                first_z = (p3-p4)*tsin[a];
                                                for (c=p6; c<360; c+=p6*2) {
                                                    _x = tcos[c]*p4 - p3;
                                                    _y = tsin[c]*p4;
                                                    crx = _x*tcos[a];
                                                    crz = - _x*tsin[a];
                                                    rel (_y, crz, crx, first_y, first_z, first_x);
                                                    first_x = crx; first_y = _y; first_z = crz;
                                                    rel (_y, crz, crx, _y, - _x*tsin[(int)(a-p6)], _x*tcos[a-static_cast<int>(p6)]);
                                                }
                                                rel (_y, crz, crx, 0, (p3-p4) * tsin[a], cry);
                                            }
                                            break;
                                    }
                                    ox = _ox;
                                    oy = _oy;
                                    oz = _oz;
                                    break;
                                case SFERA_RETICOLARE:
                                    if (p5<=0) par0 (nr_elem, pixeltype[nopix]);
                                    c = p5 * (id+1);
                                    if (c>90) c = 90;
                                    p3 = -p3;
                                    _ox = ox; ox += p0;
                                    _oy = oy; oy += p1;
                                    _oz = oz; oz += p2;
                                    for (a=0; a<360; a+=c) {
                                        first_z = p3 * tcos[c] * p4;
                                        z2 = p3 * tsin[c];
                                        first_x = z2 * tcos[a];
                                        first_y = z2 * tsin[a];
                                        for (b=2*c; b<180; b+=c) {
                                            crz = p3 * tcos[b] * p4;
                                            k1 = p3 * tsin[b];
                                            crx = k1 * tcos[a];
                                            cry = k1 * tsin[a];
                                            rel (crx, crz, cry, first_x, first_z, first_y);
                                            rel (k1 * tcos[a+c], crz, k1 * tsin[a+c], first_x, first_z, first_y);
                                            first_x = crx;
                                            first_y = cry;
                                            first_z = crz;
                                        }
                                    }
                                    ox = _ox;
                                    oy = _oy;
                                    oz = _oz;
                                    break;
            }
//nascondi:
            nr_elem++;
        }
        uneg = bkuneg;
        explode = 0;
    }
}

void Object (int tipo)
{
    int th;

    switch (tipo) {
        case 0:
            a = 0;
            while (fotty[a]!=10) {
                rel (fotty[a], fotty[a+1], fotty[a+2],
                        fotty[a+3], fotty[a+4], fotty[a+5]);
                a += 6;
            }
            rectrel (-2, 0, 4, 0.5, 0.5, 1);
            rectrel (2, 0, 4, 0.5, 0.5, 1);
            break;
        case 1:
            for (   a = static_cast<int>((SDL_GetTicks()/TICKS_PER_FRAME)%45);
                    a <= 135+(int)((SDL_GetTicks()/TICKS_PER_FRAME)%45);
                    a += 45) {
                _ox = 25 * tcos[a]; _oy = 25 * tsin[a];
                rel (-_ox, 0, _oy, _ox, 0, -_oy);
            }
            break;
        case 2:
            b = 0;
            if (/* sbf_stat&& */pix==pixel_sonante)
                b = ((SDL_GetTicks()/TICKS_PER_FRAME)%3)*15;
            for (a=b; a<=135+b; a+=45) {
                _ox = 5 * tcos[a]; _oy = 5 * tsin[a];
                rel (-_ox, 0, _oy, _ox, 0, -_oy);
            }
            rectrel (0, 0, 0, 6, 6, 1);
            if (recfile>-1) Txt ("> RECORD >", -5.5, 0, -5, 0.1, 0.15, 270, 0);
            if (globalvocfile[0]!='.') Txt ("> PLAY >", -5.5, 0, -4, 0.1, 0.15, 270, 0);
            sprintf (t, "INPUT: %s", source_name[static_cast<int>(source)]);
            Txt (t, -5.5, 0, 5, 0.1, 0.15, 270, 0);
            sprintf (t, "ACCURATEZZA: %s", record_qlty[static_cast<int>(quality)]);
            Txt (t, -5.5, 0, 4, 0.1, 0.15, 270, 0);
            if (repeat) Txt ("REPEAT", -5.5, 0, 3, 0.1, 0.15, 270, 0);
            break;
        default:
            Pixel (tipo+FRONTIER_M3);
            if (id) break;
            if (!type_mode) break; // ctrlkeys[0]&16 is scroll lock, use something else
            if (!memcmp(&subsignal[9*(tipo+FRONTIER_M3)], "TEXT-" /*"TESTO-"*/, 5/*6*/)) {
                a = subsignal[9*(tipo+FRONTIER_M3)+5/*6*/];
                memset (buffer, ' ', 512);
                sprintf (autore_forme, "TEXT-%03u.FIX", tipo-3);
                th = open (autore_forme, O_CREAT|O_RDWR /*4*/,
                        S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
                if (th>-1) {
                    buffer[32] = 0; k1 = 5.32;
                    read (th, buffer+33, 512);
                    for (c=0; c<512; c+=32) {
                        memcpy (buffer, buffer+33+c, 32);
                        if (a=='V') Txt (reinterpret_cast<char*>(buffer), -6.82, -k1, 0, 0.11, 0.14, 0, 0);
                        else if (a=='H') Txt (reinterpret_cast<char*>(buffer), -6.82, 0, k1, 0.11, 0.14, 270, 0);
                        k1 -= 0.7;
                    }
                    if (d<15||tipo==carry_type) {
                        fermo_li = 1;
                        memcpy (buffer+545, buffer+33, 512);
                        /*
                        if (tasto_premuto()) {
                            c = attendi_pressione_tasto();
                            if ((globalvocfile[0]=='.'&&extra)||!extra) play (TARGET);
                            switch (c) {
                                case 0:
                                    switch (attendi_pressione_tasto()) {
                                        case 77:
                                            if (cursore<511) cursore++;
                                            break;
                                        case 75:
                                            if (cursore) cursore--;
                                            break;
                                        case 80:
                                            if (cursore<480) cursore += 32;
                                            break;
                                        case 72:
                                            if (cursore>31) cursore -= 32;
                                            break;
                                        case 64:
                                            snapshot();
                                }
                                break;
                        case 13:
                                if (cursore<480) {
                                    cursore = (cursore/32) * 32;
                                    cursore += 32;
                                }
                                break;
                        case 8:
                                if (cursore) {
                                        cursore--;
                                        buffer[cursore+33] = 32;
                                }
                                break;
                        case 27:
                                alfin (1);
                                exit (0);
                        default:
                            if (cursore<512) {
                                if (c>='a'&&c<='z') c -= 32;
                                if (c>31&&c<97) {
                                    buffer[cursore+33] = c;
                                    if (cursore<511) cursore++;
                                }
                            }
                            */
                    }
                    if (cursore<512) {
                        if (type_this != 0) {
                            assert (type_this >= 32 && type_this <= 96);
                            buffer[cursore+33] = type_this;
                            if (cursore<511) cursore++;
                        }
                        type_this = 0;
                    }
                    //}
                    if (a=='V') Txt ("*", -6.82+(cursore%32)*0.44, (cursore/32)*0.7-5.32, 0, 0.5, 0.5, 0, 0);
                    else if (a=='H') Txt ("*", -6.82+(cursore%32)*0.44, 0, 5.32-(cursore/32)*0.7, 0.5, 0.5, 270, 0);
                    if (memcmp (buffer+33, buffer+545, 512)) {
                        lseek (th, 0, SEEK_SET);
                        write (th, buffer+33, 512);
                    }
                //}
                    close (th);
                }
                else {
                    th = creat (autore_forme, 0);
                    if (th>-1) {
                        write (th, buffer, 512);
                        close (th);
                    }
                }
        }
    }
}

// Disegna la forma di un oggetto.

char fotty[277] = {
        -6, 0, +6,          -4, 0, +8,
        -4, 0, +8,          -2, 0, +8,
        -2, 0, +8,          -2, 0, +6,
        -2, 0, +6,           0, 0, +7,
         0, 0, +7,          +2, 0, +6,
        +2, 0, +6,          +2, 0, +8,
        +2, 0, +8,          +4, 0, +8,
        +4, 0, +8,          +6, 0, +6,
        +6, 0, +6,          +6, 0, +4,
        +6, 0, +4,          +4, 0, +4,
        +4, 0, +4,          +4, 0, +2,
        +4, 0, +2,          +2, 0, +1,
        +2, 0, +1,          +4, 0,  0,
        +4, 0,  0,          +5, 0, -2,
        +5, 0, -2,          +4, 0, -4,
        +4, 0, -4,          +4, 0, -6,
        +4, 0, -6,          +2, 0, -8,
        +2, 0, -8,           0, 0, -7,
         0, 0, -7,          -2, 0, -8,
        -2, 0, -8,          -4, 0, -6,
        -4, 0, -6,          -4, 0, -4,
        -4, 0, -4,          -5, 0, -2,
        -5, 0, -2,          -4, 0,  0,
        -4, 0,  0,          -2, 0, +1,
        -2, 0, +1,          -4, 0, +2,
        -4, 0, +2,          -4, 0, +4,
        -4, 0, +4,          -6, 0, +4,
        -6, 0, +4,          -6, 0, +6,
        -2, 0,  0,          -1, 0, -2,
        -1, 0, -2,          -2, 0, -4,
        -2, 0, -4,           0, 0, -5,
         0, 0, -5,          +2, 0, -4,
        +2, 0, -4,          +1, 0, -2,
        +1, 0, -2,          +2, 0,  0,
        -4, 0, -4,          -2, 0, -4,
        +2, 0, -4,          +4, 0, -4,
         0, 0, -5,           0, 0, -7,
        -2, 0, +1,          -2, 0, +3,
        -2, 0, +3,           0, 0, +4,
         0, 0, +4,          +2, 0, +3,
        +2, 0, +3,          +2, 0, +1,
        +2, 0, +1,           0, 0,  0,
         0, 0,  0,          -2, 0, +1,
        -1, 0, +3,          +1, 0, +3,
        +1, 0, +3,           0, 0, +1,
         0, 0, +1,          -1, 0, +3,
10 };