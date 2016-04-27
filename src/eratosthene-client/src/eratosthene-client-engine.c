/*
 *  eratosthene-suite - geodetic system
 *
 *      Nils Hamel - nils.hamel@bluewin.ch
 *      Copyright (c) 2016 EPFL CDH DHLAB
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

    # include "eratosthene-client-engine.h"

/*
    source - ugly global variable (GLUT callbacks)
 */

    er_engine_t er_engine = ER_ENGINE_C;

/*
    source - rendering engine
 */

    void er_engine_main( le_char_t const * const er_ip, le_sock_t const er_port ) {

        /* Thread variables */
        pthread_t er_secondary;

        /* Assign server address */
        strcpy( ( char * ) er_engine.eg_ip, ( char * ) er_ip );

        /* Assign server port */
        er_engine.eg_port = er_port;

        /* Setting windows parameteres */
        glutInitWindowSize( glutGet( GLUT_SCREEN_WIDTH ), glutGet( GLUT_SCREEN_HEIGHT ) );

        /* Create rendering engine window */
        glutCreateWindow( "eratosthene-client" );

        /* Display rendering engine window in fullscreen */
        glutFullScreen();

        /* Hide mouse cursor */
        glutSetCursor( GLUT_CURSOR_NONE );

        /* Rendering engine display configuration */
        glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL );

        /* Setting GLUT options */
        glutSetOption( GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION );

        /* Setting color clear value */
        glClearColor( 0.0, 0.0, 0.0, 0.0 );

        /* Setting depth clear value */
        glClearDepth( 1.0 );

        /* Setting depth configuration */
        glEnable( GL_DEPTH_TEST );
        glDepthFunc( GL_LEQUAL );
        glDepthMask( GL_TRUE );

        /* Shade model configuration */
        glShadeModel( GL_SMOOTH );

        /* Create model */
        er_engine.eg_model = er_model_create( 512 );

        /* Enable vertex and color arrays */
        glEnableClientState( GL_VERTEX_ARRAY );
        glEnableClientState( GL_COLOR_ARRAY  );

        /* Engine secondary loop */
        pthread_create( & er_secondary, NULL, & er_engine_second, NULL );

        /* Engine primary loop */
        glutDisplayFunc      ( er_engine_render  );
        glutIdleFunc         ( er_engine_render  );
        glutReshapeFunc      ( er_engine_reshape );
        glutKeyboardFunc     ( er_engine_keybd   );
        glutMouseFunc        ( er_engine_mouse   );
        glutMotionFunc       ( er_engine_move    );
        glutPassiveMotionFunc( er_engine_move    );

        /* Engine primary loop */
        glutMainLoop();

        /* Engine secondary loop */
        pthread_cancel( er_secondary );

        /* Disable vertex and color arrays */
        glDisableClientState( GL_COLOR_ARRAY  );
        glDisableClientState( GL_VERTEX_ARRAY );

        /* Delete model */
        er_model_delete( & ( er_engine.eg_model ) );

    }

    void * er_engine_second( void * er_void ) {

        /* Engine secondary loop */
        for ( ; ; sleep( 0.25 ) ) er_engine_update();

    }

/*
    source - engine callbacks - primary
 */

    void er_engine_render( void ) {

        /* Recompute near/far planes */
        er_engine_reshape( glutGet( GLUT_SCREEN_WIDTH ), glutGet( GLUT_SCREEN_HEIGHT ) );

        /* Clear color and depth buffers */
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        /* Configure points display */
        glPointSize( er_engine.eg_point );

        /* Push matrix */
        glPushMatrix(); {

            /* Motion management - translation */
            glTranslatef( 0.0,

                + sin( er_engine.eg_vgam * ER_D2R ) * er_engine.eg_valt,
                - cos( er_engine.eg_vgam * ER_D2R ) * er_engine.eg_valt

            );

            /* Motion management - rotations */
            glRotatef( +er_engine.eg_vgam, 1.0, 0.0, 0.0 );
            glRotatef( +er_engine.eg_vazm, 0.0, 0.0, 1.0 );
            glRotatef( +er_engine.eg_vlat, 1.0, 0.0, 0.0 );
            glRotatef( -er_engine.eg_vlon, 0.0, 1.0, 0.0 );

            /* Display earth model */
            er_model_main( & ( er_engine.eg_model ) );

            /* Earth frame - orientation */
            glRotatef( 90.0, 1.0, 0.0, 0.0 );

            /* Earth frame - color */
            glColor3f( 0.3, 0.32, 0.4 );

            /* Earth model variables */
            GLUquadricObj * er_earth = gluNewQuadric();

            /* Configure quadric */
            gluQuadricDrawStyle( er_earth, GLU_LINE );

            /* Draw quadric */
            gluSphere( er_earth, ER_ERA, 360, 180 );

            /* Delete quadric */
            gluDeleteQuadric( er_earth );

        /* Pop matrix */
        } glPopMatrix();

        /* Swap buffers */
        glutSwapBuffers();

    }

    void er_engine_update( void ) {

        /* Update ranges */
        er_engine_range();

        /* Update model */
        er_model_update( & ( er_engine.eg_model ), er_engine.eg_time, er_engine.eg_vlon * ER_D2R, er_engine.eg_vlat * ER_D2R, er_engine.eg_valt * 1000 );

        /* Query model */
        er_model_query( & ( er_engine.eg_model ), ( le_char_t * ) er_engine.eg_ip, er_engine.eg_port );

    }

/*
    source - engine callbacks - reshape
 */

    void er_engine_reshape( int er_width, int er_height ) {

        /* Reset viewport */
        glViewport( 0, 0, er_width, er_height );

        /* Matrix mode to projection */
        glMatrixMode( GL_PROJECTION );

        /* Set projection matrix to identity */
        glLoadIdentity();

        /* Compute projectio matrix */
        gluPerspective( 45, ( float ) er_width / er_height, 0.1, er_engine.eg_valt - ER_ER2 );

        /* Matrix mode to modelview */
        glMatrixMode( GL_MODELVIEW );

        /* Set model view matrix to identity */
        glLoadIdentity();
        

        /* New */
        glScaled( er_engine.eg_vscl, er_engine.eg_vscl, er_engine.eg_vscl );

    }

/*
    source - engine callbacks - keyboard
 */

    void er_engine_keybd( unsigned char er_keycode, int er_x, int er_y ) {

        /* Switch on keycode */
        switch( er_keycode ) {

            /* Escape */
            case ( 27 ) : {

                /* Leave GLUT events management loop */
                glutLeaveMainLoop();

            } break;

            /* Point size control */
            case ( 'w' ) : {

                /* Update point size */
                er_engine.eg_point = ( er_engine.eg_point < 8 ) ? er_engine.eg_point + 1 : 8;

            } break;

            /* Point size control */
            case ( 'q' ) : {

                /* Update point size */
                er_engine.eg_point = ( er_engine.eg_point > 1 ) ? er_engine.eg_point - 1 : 1;

            } break;

            /* Reset azimuth and gamma */
            case ( 'e' ) : {

                /* Reset variables */
                er_engine.eg_vazm = 0.0;
                er_engine.eg_vgam = 0.0;

            } break;

            /* Reset point of view */
            case ( 'r' ) : {

                /* Reset variables */
                er_engine.eg_vlon = 0.0;
                er_engine.eg_vlat = 0.0;
                er_engine.eg_valt = 1.5 * ER_ERA;
                er_engine.eg_vazm = 0.0;
                er_engine.eg_vgam = 0.0;

            } break;

            case ( 'o' ) : { er_engine.eg_vscl *= 1.10; } break;
            case ( 'p' ) : { er_engine.eg_vscl *= 0.91; } break;

        };

    }

/*
    source - engine callbacks - mouse
 */

    void er_engine_mouse( int er_button, int er_state, int er_x, int er_y ) {

        /* Update engine handle */
        er_engine.eg_button = er_button;
        er_engine.eg_state  = er_state;
        er_engine.eg_x      = er_x;
        er_engine.eg_y      = er_y;
        er_engine.eg_u      = er_x;
        er_engine.eg_v      = er_y;

        /* Mouse event switch */
        if ( ( er_engine.eg_button == 3 ) && ( er_engine.eg_state == GLUT_DOWN ) ) {

            /* Update altitude */
            er_engine.eg_valt += ( er_engine.eg_valt - ER_ERA ) * 0.05;

        }

        /* Mouse event switch */
        if ( ( er_engine.eg_button == 4 ) && ( er_engine.eg_state == GLUT_DOWN ) ) {

            /* Update altitude */
            er_engine.eg_valt -= ( er_engine.eg_valt - ER_ERA ) * 0.05;

        }

    }

    void er_engine_move( int er_x, int er_y ) {

        /* Check mouse state */
        if ( er_engine.eg_state == GLUT_DOWN ) {

            /* Update engine handle */
            er_engine.eg_u = er_x;
            er_engine.eg_v = er_y;

        }

        /* Mouse event switch */
        if ( ( er_engine.eg_button == GLUT_LEFT_BUTTON ) && ( er_engine.eg_state == GLUT_DOWN ) ) {

            /* Update longitude and latitude */
            er_engine.eg_vlon -= ER_ENGINE_MOVE * ( er_engine.eg_u - er_engine.eg_x );
            er_engine.eg_vlat += ER_ENGINE_MOVE * ( er_engine.eg_v - er_engine.eg_y );

        }

        /* Mouse switch event */
        if ( ( er_engine.eg_button == GLUT_MIDDLE_BUTTON ) && ( er_engine.eg_state == GLUT_DOWN ) ) {

            /* Update longitude and latitude */
            er_engine.eg_vlon += ER_ENGINE_MOVE * ( er_engine.eg_v - er_engine.eg_y ) * sin( er_engine.eg_vazm * ER_D2R );
            er_engine.eg_vlat += ER_ENGINE_MOVE * ( er_engine.eg_v - er_engine.eg_y ) * cos( er_engine.eg_vazm * ER_D2R );

        }

        /* Mouse event switch */
        if ( ( er_engine.eg_button == GLUT_RIGHT_BUTTON ) && ( er_engine.eg_state == GLUT_DOWN ) ) {

            /* Update azimuth and gamma angles */
            er_engine.eg_vazm -= ER_ENGINE_MOVE * ( er_engine.eg_u - er_engine.eg_x );
            er_engine.eg_vgam -= ER_ENGINE_MOVE * ( er_engine.eg_v - er_engine.eg_y ) * 2.0;

        }

    }

/*
    source - engine callbacks - ranges
 */

    void er_engine_range() {

        /* Angle ranges - cyclic */
        if ( er_engine.eg_vlon > +180.0 ) er_engine.eg_vlon -= +360.0;
        if ( er_engine.eg_vlon < -180.0 ) er_engine.eg_vlon += +360.0;
        if ( er_engine.eg_vlat > + 90.0 ) er_engine.eg_vlat  = + 90.0;
        if ( er_engine.eg_vlat < - 90.0 ) er_engine.eg_vlat  = - 90.0;
        if ( er_engine.eg_vazm > +360.0 ) er_engine.eg_vazm -= +360.0;
        if ( er_engine.eg_vazm < -360.0 ) er_engine.eg_vazm += +360.0;

        /* Angles ranges - clamp */
        if ( er_engine.eg_vgam <  -90.0 ) er_engine.eg_vgam = - 90.0;
        if ( er_engine.eg_vgam >  + 0.0 ) er_engine.eg_vgam = +  0.0;

        /* Parameter ranges - clamp */
        if ( er_engine.eg_valt < ER_ERL ) er_engine.eg_valt = ER_ERL;
        if ( er_engine.eg_valt > ER_ERU ) er_engine.eg_valt = ER_ERU;

    }
