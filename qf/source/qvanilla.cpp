//****************************************************************************
// Product: QF/C++
// Last Updated for Version: 5.1.1
// Date of the Last Update:  Oct 07, 2013
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
#include "qf_pkg.h"
#include "qassert.h"

/// \file
/// \ingroup qf
/// \brief "vanilla" cooperative kernel,
/// QActive::start(), QActive::stop(), and QF::run() implementation.

namespace QP {

Q_DEFINE_THIS_MODULE("qvanilla")

// Package-scope objects -----------------------------------------------------
extern "C" {
#if (QF_MAX_ACTIVE <= 8)
    QPSet8  QF_readySet_;                                  // ready set of AOs
#else
    QPSet64 QF_readySet_;                                  // ready set of AOs
#endif

uint8_t volatile QF_currPrio_;            ///< current task/interrupt priority
uint8_t volatile QF_intNest_;                     ///< interrupt nesting level

}                                                                // extern "C"

//............................................................................
void QF::init(void) {
    // nothing to do for the "vanilla" kernel
}
//............................................................................
void QF::stop(void) {
    onCleanup();                                           // cleanup callback
    // nothing else to do for the "vanilla" kernel
}
//............................................................................
int16_t QF::run(void) {
    onStartup();                                           // startup callback

    for (;;) {                                           // the bacground loop
        QF_INT_DISABLE();
        if (QF_readySet_.notEmpty()) {
            uint8_t p = QF_readySet_.findMax();
            QActive *a = active_[p];
            QF_currPrio_ = p;                     // save the current priority
            QF_INT_ENABLE();

            QEvt const *e = a->get_();       // get the next event for this AO
            a->dispatch(e);                         // dispatch evt to the HSM
            gc(e);       // determine if event is garbage and collect it if so
        }
        else {
            onIdle();                                            // see NOTE01
        }
    }

#ifdef __GNUC__                                               // GNU compiler?
    return static_cast<int16_t>(0);
#endif
}
//............................................................................
void QActive::start(uint8_t const prio,
                    QEvt const *qSto[], uint32_t const qLen,
                    void * const stkSto, uint32_t const,
                    QEvt const * const ie)
{
    Q_REQUIRE((u8_0 < prio) && (prio <= static_cast<uint8_t>(QF_MAX_ACTIVE))
              && (stkSto == null_void));      // does not need per-actor stack

    m_eQueue.init(qSto, static_cast<QEQueueCtr>(qLen));  // initialize QEQueue
    m_prio = prio;                // set the QF priority of this active object
    QF::add_(this);                     // make QF aware of this active object
    this->init(ie);               // execute initial transition (virtual call)

    QS_FLUSH();                          // flush the trace buffer to the host
}
//............................................................................
void QActive::stop(void) {
    QF::remove_(this);
}

}                                                              // namespace QP

//****************************************************************************
// NOTE01:
// QF::onIdle() must be called with interrupts DISABLED because the
// determination of the idle condition (no events in the queues) can change
// at any time by an interrupt posting events to a queue. The QF::onIdle()
// MUST enable interrups internally, perhaps at the same time as putting the
// CPU into a power-saving mode.
//

