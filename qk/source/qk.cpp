//****************************************************************************
// Product: QK/C++
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
#include "qk_pkg.h"
#include "qassert.h"

/// \file
/// \ingroup qk
/// \brief QK_readySet_, QK_currPrio_, and QK_intNest_ definitions and
/// QF::init(), QF::run(), QF::stop(),
/// QActive::start(), QActive::stop() implementations.

// Public-scope objects ------------------------------------------------------
extern "C" {
#if (QF_MAX_ACTIVE <= 8)
    QP::QPSet8  QK_readySet_;                              // ready set of AOs
#else
    QP::QPSet64 QK_readySet_;                              // ready set of AOs
#endif
                                         // start with the QK scheduler locked
uint8_t volatile QK_currPrio_ = static_cast<uint8_t>(QF_MAX_ACTIVE + 1);
uint8_t volatile QK_intNest_;                 // start with nesting level of 0

}                                                                // extern "C"

namespace QP {

Q_DEFINE_THIS_MODULE("qk")

//............................................................................
void QF::init(void) {
    QK_init();           // QK initialization ("C" linkage, might be assembly)
}
//............................................................................
void QF::stop(void) {
    QF::onCleanup();                                       // cleanup callback
    // nothing else to do for the QK preemptive kernel
}
//............................................................................
static void initialize(void) {
    QK_currPrio_ = static_cast<uint8_t>(0);   // priority for the QK idle loop
    uint8_t p = QK_schedPrio_();
    if (p != static_cast<uint8_t>(0)) {
        QK_sched_(p);                    // process all events produced so far
    }
}
//............................................................................
int16_t QF::run(void) {
    QF_INT_DISABLE();
    initialize();
    onStartup();                                           // startup callback
    QF_INT_ENABLE();

    for (;;) {                                             // the QK idle loop
        QK::onIdle();                        // invoke the QK on-idle callback
    }

#ifdef __GNUC__                                               // GNU compiler?
    return static_cast<int16_t>(0);
#endif
}
//............................................................................
void QActive::start(uint8_t const prio,
                   QEvt const *qSto[], uint32_t const qLen,
                   void * const stkSto, uint32_t const stkSize,
                   QEvt const * const ie)
{
    Q_REQUIRE((static_cast<uint8_t>(0) < prio)
              && (prio <= static_cast<uint8_t>(QF_MAX_ACTIVE)));

    m_eQueue.init(qSto, static_cast<QEQueueCtr>(qLen));    // initialize queue
    m_prio = prio;
    QF::add_(this);                     // make QF aware of this active object

#if defined(QK_TLS) || defined(QK_EXT_SAVE)
    // in the QK port the parameter stkSize is used as the thread flags
    m_osObject = static_cast<uint8_t>(stkSize);   // m_osObject contains flags

    // in the QK port the parameter stkSto is used as the thread-local-storage
    m_thread   = stkSto;   // contains the pointer to the thread-local-storage
#else
    Q_ASSERT((stkSto == static_cast<void *>(0))
             && (stkSize == static_cast<uint32_t>(0)));
#endif

    this->init(ie);               // execute initial transition (virtual call)

    QS_FLUSH();                          // flush the trace buffer to the host
}
//............................................................................
void QActive::stop(void) {
    QF::remove_(this);                // remove this active object from the QF
}

}                                                              // namespace QP
