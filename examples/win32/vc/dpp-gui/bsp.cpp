//****************************************************************************
// Product: DPP example
// Last Updated for Version: 5.1.1
// Date of the Last Update:  Oct 10, 2013
//
//                    Q u a n t u m     L e a P s
//                    ---------------------------
//                    innovating embedded systems
//
// Copyright (C) 2002-2013 Quantum Leaps, LLC. All rights reserved.
//
// This program is open source software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Alternatively, this program may be distributed and modified under the
// terms of Quantum Leaps commercial licenses, which expressly supersede
// the GNU General Public License and are specifically designed for
// licensees interested in retaining the proprietary status of their code.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//
// Contact information:
// Quantum Leaps Web sites: http://www.quantum-leaps.com
//                          http://www.state-machine.com
// e-mail:                  info@quantum-leaps.com
//****************************************************************************
#include "qp_port.h"
#include "dpp.h"
#include "bsp.h"

#include "win32_gui.h"         // Win32 GUI elements for embedded front panels
#include "resource.h"    // GUI resource IDs generated by the resource editior

#include <stdio.h>                                        // for _snprintf_s()

namespace DPP {

Q_DEFINE_THIS_FILE

// local variables -----------------------------------------------------------
static HINSTANCE l_hInst;                         // this application instance
static HWND      l_hWnd;                                 // main window handle
static LPSTR     l_cmdLine;                         // the command line string

static SegmentDisplay   l_philos;       // SegmentDisplay to show Philo status
static OwnerDrawnButton l_pauseBtn;                      // owner-drawn button

static unsigned  l_rnd;                                         // random seed

#ifdef Q_SPY
    enum {
        PHILO_STAT = QP::QS_USER
    };
    static uint8_t const l_clock_tick = 0U;
#endif

// Local functions -----------------------------------------------------------
static LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg,
                                WPARAM wParam, LPARAM lParam);

//............................................................................
extern "C" int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst,
                              LPSTR cmdLine, int iCmdShow)
{
    (void)hPrevInst;          // avoid compiler warning about unused parameter

    l_hInst   = hInst;                        // save the application instance
    l_cmdLine = cmdLine;                       // save the command line string

    // create the main custom dialog window
    HWND hWnd = CreateCustDialog(hInst, IDD_APPLICATION, NULL,
                                 &WndProc, "QP_APP");
    ShowWindow(hWnd, iCmdShow);                        // show the main window

    // enter the message loop...
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}
//............................................................................
int_t main_gui(void);                             // prototype for appThread()

// thread function for running the application main()
static DWORD WINAPI appThread(LPVOID par) {
    (void)par;                                             // unused parameter
    return main_gui();                               // run the QF application
}
//............................................................................
static LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg,
                                    WPARAM wParam, LPARAM lParam)
{
    switch (iMsg) {

        // Perform initialization upon cration of the main dialog window
        // NOTE: Any child-windows are NOT created yet at this time, so
        // the GetDlgItem() function can't be used (it will return NULL).
        //
        case WM_CREATE: {
            l_hWnd = hWnd;                           // save the window handle

            // initialize the owner-drawn buttons
            // NOTE: must be done *before* the first drawing of the buttons,
            // so WM_INITDIALOG is too late.
            //
            l_pauseBtn.init(LoadBitmap(l_hInst, MAKEINTRESOURCE(IDB_BTN_UP)),
                            LoadBitmap(l_hInst, MAKEINTRESOURCE(IDB_BTN_DWN)),
                            LoadCursor(NULL, IDC_HAND));
            return 0;
        }

        // Perform initialization after all child windows have been created
        case WM_INITDIALOG: {

            l_philos.init(N_PHILO,        // N_PHILO "segments" for the Philos
                          3U);       // 3 bitmaps (for thinking/hungry/eating)
            l_philos.initSegment(0U, GetDlgItem(hWnd, IDC_PHILO_0));
            l_philos.initSegment(1U, GetDlgItem(hWnd, IDC_PHILO_1));
            l_philos.initSegment(2U, GetDlgItem(hWnd, IDC_PHILO_2));
            l_philos.initSegment(3U, GetDlgItem(hWnd, IDC_PHILO_3));
            l_philos.initSegment(4U, GetDlgItem(hWnd, IDC_PHILO_4));
            l_philos.initBitmap(0U,
                LoadBitmap(l_hInst, MAKEINTRESOURCE(IDB_THINKING)));
            l_philos.initBitmap(1U,
                LoadBitmap(l_hInst, MAKEINTRESOURCE(IDB_HUNGRY)));
            l_philos.initBitmap(2U,
                LoadBitmap(l_hInst, MAKEINTRESOURCE(IDB_EATING)));

            // --> QP: spawn the application thread to run main()
            Q_ALLEGE(CreateThread(NULL, 0, &appThread, NULL, 0, NULL)
                     != (HANDLE)0);
            return 0;
        }

        case WM_DESTROY: {
            BSP_terminate(0);
            return 0;
        }

        // commands from regular buttons and menus...
        case WM_COMMAND: {
            SetFocus(hWnd);
            switch (wParam) {
                case IDOK:
                case IDCANCEL: {
                    //QP::QF::PUBLISH(Q_NEW(QP::QEvt, TERMINATE_SIG), (void *)0);
                    BSP_terminate(0);
                    break;
                }
            }
            return 0;
        }

        // owner-drawn buttons...
        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
            switch (pdis->CtlID) {
                case IDC_PAUSE: {                  // PAUSE owner-drawn button
                    static QP::QEvt pe = QEVT_INITIALIZER(PAUSE_SIG);
                    switch (l_pauseBtn.draw(pdis)) {
                        case OwnerDrawnButton::BTN_DEPRESSED: {
                            AO_Table->POST(&pe, (void *)0);
                            break;
                        }
                        case OwnerDrawnButton::BTN_RELEASED: {
                            AO_Table->POST(&pe, (void *)0);
                            break;
                        }
                    }
                    break;
                }
            }
            return 0;
        }

        // mouse input...
        case WM_MOUSEWHEEL: {
            return 0;
        }

        // keyboard input...
        case WM_KEYDOWN: {
            return 0;
        }

    }
    return DefWindowProc(hWnd, iMsg, wParam, lParam) ;
}
//............................................................................
void BSP_init(void) {
    Q_ALLEGE(QS_INIT(l_cmdLine));
    QS_RESET();
    QS_OBJ_DICTIONARY(&l_clock_tick);     // must be called *after* QF::init()
    QS_USR_DICTIONARY(PHILO_STAT);
}
//............................................................................
void BSP_terminate(int16_t result) {
    QP::QF::stop();
    PostQuitMessage(result);
}
//............................................................................
void BSP_displayPhilStat(uint8_t n, char const *stat) {
    UINT bitmapNum = 0;

    Q_REQUIRE(n < N_PHILO);

    switch (stat[0]) {
        case 't': bitmapNum = 0U; break;
        case 'h': bitmapNum = 1U; break;
        case 'e': bitmapNum = 2U; break;
        default: Q_ERROR();  break;
    }
    // set the "segment" # n to the bitmap # 'bitmapNum'
    l_philos.setSegment((UINT)n, bitmapNum);

    QS_BEGIN(PHILO_STAT, AO_Philo[n])     // application-specific record begin
        QS_U8(1, n);                                     // Philosopher number
        QS_STR(stat);                                    // Philosopher status
    QS_END()
}
//............................................................................
void BSP_displayPaused(uint8_t paused) {
    char buf[16];
    LoadString(l_hInst,
        (paused != 0U) ? IDS_PAUSED : IDS_RUNNING, buf, Q_DIM(buf));
    SetDlgItemText(l_hWnd, IDC_PAUSED, buf);
}
//............................................................................
uint32_t BSP_random(void) {     // a very cheap pseudo-random-number generator
    // "Super-Duper" Linear Congruential Generator (LCG)
    // LCG(2^32, 3*7*11*13*23, 0, seed)
    //
    l_rnd = l_rnd * (3U*7U*11U*13U*23U);
    return l_rnd >> 8;
}
//............................................................................
void BSP_randomSeed(uint32_t seed) {
    l_rnd = seed;
}

} // namespace DPP

//****************************************************************************

namespace QP {

//............................................................................
void QF::onStartup(void) {
    QF_setTickRate(DPP::BSP_TICKS_PER_SEC);       // set the desired tick rate
}
//............................................................................
void QF::onCleanup(void) {
}
//............................................................................
void QF_onClockTick(void) {
    QF::TICK(&DPP::l_clock_tick);      // perform the QF clock tick processing
}
//............................................................................
extern "C" void Q_onAssert(char const Q_ROM * const file, int line) {
    char message[80];
    QF::stop();                                                // stop ticking
    _snprintf_s(message, Q_DIM(message) - 1, _TRUNCATE,
                "Assertion failed in module %hs line %d", file, line);
    MessageBox(DPP::l_hWnd, message, "!!! ASSERTION !!!",
               MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
    PostQuitMessage(-1);
}

//----------------------------------------------------------------------------
#ifdef Q_SPY                                            // define QS callbacks

#include "qspy.h"

#include <time.h>

static uint8_t l_running;

//............................................................................
static DWORD WINAPI idleThread(LPVOID par) {   // signature for CreateThread()
    (void)par;

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);
    l_running = (uint8_t)1;
    while (l_running) {
        uint16_t nBytes = 256U;
        uint8_t const *block;
        QF_CRIT_ENTRY(dummy);
        block = QS::getBlock(&nBytes);
        QF_CRIT_EXIT(dummy);
        if (block != (uint8_t *)0) {
            QSPY_parse(block, nBytes);
        }
        Sleep(10U);                                        // wait for a while
    }
    return 0;                                                // return success
}
//............................................................................
static int custParserFun(QSpyRecord * const qrec) {
    static uint32_t mpool0;
    int ret = 0;                              // perform standard QSPY parsing
    switch (qrec->rec) {
        case QS_QF_MPOOL_INIT: {                    // example record to parse
            if (mpool0 == 0U) {                                   // MPool[0]?
                mpool0 = (uint32_t)QSpyRecord_getUint64(qrec,QS_OBJ_PTR_SIZE);
            }
            break;
        }
        case QS_QF_MPOOL_GET: {                     // example record to parse
            int nFree;
            uint32_t mpool;
            (void)QSpyRecord_getUint32(qrec, QS_TIME_SIZE);
            mpool = (uint32_t)QSpyRecord_getUint64(qrec, QS_OBJ_PTR_SIZE);
            nFree = (int)QSpyRecord_getUint32(qrec, QF_MPOOL_CTR_SIZE);
            (void)QSpyRecord_getUint32(qrec, QF_MPOOL_CTR_SIZE);       // nMin
            if (QSpyRecord_OK(qrec) && (mpool == mpool0)) {       // MPool[0]?
                SetDlgItemInt(DPP::l_hWnd, IDC_MPOOL0, nFree, FALSE);
                ret = 0;                // don't perform standard QSPY parsing
            }
            break;
        }
    }
    return ret;
}
//............................................................................
bool QS::onStartup(void const *arg) {
    static uint8_t qsBuf[4*1024];                 // 4K buffer for Quantum Spy
    initBuf(qsBuf, sizeof(qsBuf));

    // here 'arg' is ignored, but this command-line parameter can be used
    // to setup the QSP_config(), to set up the QS filters, or for any
    // other purpose.
    //
    (void)arg;

    QSPY_config(QP_VERSION,         // version
                QS_OBJ_PTR_SIZE,    // objPtrSize
                QS_FUN_PTR_SIZE,    // funPtrSize
                QS_TIME_SIZE,       // tstampSize
                Q_SIGNAL_SIZE,      // sigSize,
                QF_EVENT_SIZ_SIZE,  // evtSize
                QF_EQUEUE_CTR_SIZE, // queueCtrSize
                QF_MPOOL_CTR_SIZE,  // poolCtrSize
                QF_MPOOL_SIZ_SIZE,  // poolBlkSize
                QF_TIMEEVT_CTR_SIZE,// tevtCtrSize
                (void *)0,          // matFile,
                (void *)0,          // mscFile
                &custParserFun);    // customized parser function

    QS_FILTER_ON(QS_ALL_RECORDS);

//    QS_FILTER_OFF(QS_QEP_STATE_EMPTY);
//    QS_FILTER_OFF(QS_QEP_STATE_ENTRY);
//    QS_FILTER_OFF(QS_QEP_STATE_EXIT);
//    QS_FILTER_OFF(QS_QEP_STATE_INIT);
//    QS_FILTER_OFF(QS_QEP_INIT_TRAN);
//    QS_FILTER_OFF(QS_QEP_INTERN_TRAN);
//    QS_FILTER_OFF(QS_QEP_TRAN);
//    QS_FILTER_OFF(QS_QEP_IGNORED);
//    QS_FILTER_OFF(QS_QEP_DISPATCH);
//    QS_FILTER_OFF(QS_QEP_UNHANDLED);

    QS_FILTER_OFF(QS_QF_ACTIVE_ADD);
    QS_FILTER_OFF(QS_QF_ACTIVE_REMOVE);
    QS_FILTER_OFF(QS_QF_ACTIVE_SUBSCRIBE);
    QS_FILTER_OFF(QS_QF_ACTIVE_UNSUBSCRIBE);
    QS_FILTER_OFF(QS_QF_ACTIVE_POST_FIFO);
    QS_FILTER_OFF(QS_QF_ACTIVE_POST_LIFO);
    QS_FILTER_OFF(QS_QF_ACTIVE_GET);
    QS_FILTER_OFF(QS_QF_ACTIVE_GET_LAST);
    QS_FILTER_OFF(QS_QF_EQUEUE_INIT);
    QS_FILTER_OFF(QS_QF_EQUEUE_POST_FIFO);
    QS_FILTER_OFF(QS_QF_EQUEUE_POST_LIFO);
    QS_FILTER_OFF(QS_QF_EQUEUE_GET);
    QS_FILTER_OFF(QS_QF_EQUEUE_GET_LAST);
//    QS_FILTER_OFF(QS_QF_MPOOL_INIT);
//    QS_FILTER_OFF(QS_QF_MPOOL_GET);
    QS_FILTER_OFF(QS_QF_MPOOL_PUT);
    QS_FILTER_OFF(QS_QF_PUBLISH);
    QS_FILTER_OFF(QS_QF_NEW);
    QS_FILTER_OFF(QS_QF_GC_ATTEMPT);
    QS_FILTER_OFF(QS_QF_GC);
    QS_FILTER_OFF(QS_QF_TICK);
    QS_FILTER_OFF(QS_QF_TIMEEVT_ARM);
    QS_FILTER_OFF(QS_QF_TIMEEVT_AUTO_DISARM);
    QS_FILTER_OFF(QS_QF_TIMEEVT_DISARM_ATTEMPT);
    QS_FILTER_OFF(QS_QF_TIMEEVT_DISARM);
    QS_FILTER_OFF(QS_QF_TIMEEVT_REARM);
    QS_FILTER_OFF(QS_QF_TIMEEVT_POST);
    QS_FILTER_OFF(QS_QF_CRIT_ENTRY);
    QS_FILTER_OFF(QS_QF_CRIT_EXIT);
    QS_FILTER_OFF(QS_QF_ISR_ENTRY);
    QS_FILTER_OFF(QS_QF_ISR_EXIT);

    return CreateThread(NULL, 1024, &idleThread, (void *)0, 0, NULL)
             != (HANDLE)0;    // return the status of creating the idle thread
}
//............................................................................
void QS::onCleanup(void) {
    l_running = (uint8_t)0;
    QSPY_stop();
}
//............................................................................
void QS::onFlush(void) {
    uint16_t nBytes = 1024U;
    uint8_t const *block;
    while ((block = getBlock(&nBytes)) != (uint8_t *)0) {
        QSPY_parse(block, nBytes);
        nBytes = 1024U;
    }
}
//............................................................................
QSTimeCtr QS::onGetTime(void) {
    return (QSTimeCtr)clock();
}
//............................................................................
void QSPY_onPrintLn(void) {
    OutputDebugString(QSPY_line);
    OutputDebugString("\n");
}
#endif                                                                // Q_SPY
//----------------------------------------------------------------------------

} // namespace QP
