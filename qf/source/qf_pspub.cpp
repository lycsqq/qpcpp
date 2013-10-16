//****************************************************************************
// Product: QF/C++
// Last Updated for Version: 5.1.0
// Date of the Last Update:  Sep 28, 2013
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
/// \brief QF::publish() implementation.

namespace QP {

Q_DEFINE_THIS_MODULE("qf_pspub")

//............................................................................
#ifndef Q_SPY
void QF::publish(QEvt const * const e) {
#else
void QF::publish(QEvt const * const e, void const * const sender) {
#endif
         // make sure that the published signal is within the configured range
    Q_REQUIRE(static_cast<enum_t>(e->sig) < QF_maxSignal_);

    QF_CRIT_STAT_
    QF_CRIT_ENTRY_();

    QS_BEGIN_NOCRIT_(QS_QF_PUBLISH, null_void, null_void)
        QS_TIME_();                                           // the timestamp
        QS_OBJ_(sender);                                  // the sender object
        QS_SIG_(e->sig);                            // the signal of the event
        QS_2U8_(e->poolId_, e->refCtr_);        // pool Id & refCtr of the evt
    QS_END_NOCRIT_()

    if (e->poolId_ != u8_0) {                        // is it a dynamic event?
        QF_EVT_REF_CTR_INC_(e);     // increment the reference counter, NOTE01
    }
    QF_CRIT_EXIT_();

#if (QF_MAX_ACTIVE <= 8)
    uint8_t tmp = QF_PTR_AT_(QF_subscrList_, e->sig).m_bits[0];
    while (tmp != u8_0) {
        uint8_t p = QF_LOG2(tmp);
        tmp &= Q_ROM_BYTE(QF_invPwr2Lkup[p]);      // clear the subscriber bit
        Q_ASSERT(active_[p] != static_cast<QActive *>(0));//must be registered

                           // POST() asserts internally if the queue overflows
        (void)active_[p]->POST(e, sender);
    }
#else
                                                // number of bytes in the list
    uint8_t i = QF_SUBSCR_LIST_SIZE;
    do {                       // go through all bytes in the subsciption list
        --i;
        uint8_t tmp = QF_PTR_AT_(QF_subscrList_, e->sig).m_bits[i];
        while (tmp != u8_0) {
            uint8_t p = QF_LOG2(tmp);
            tmp &= Q_ROM_BYTE(QF_invPwr2Lkup[p]);  // clear the subscriber bit
                                                        // adjust the priority
            p = static_cast<uint8_t>(p + static_cast<uint8_t>(i << 3));
            Q_ASSERT(active_[p] != static_cast<QActive *>(0));   // registered

                           // POST() asserts internally if the queue overflows
            (void)active_[p]->POST(e, sender);               // asserting post
        }
    } while (i != u8_0);
#endif

    gc(e);                            // run the garbage collector, see NOTE01
}

}                                                              // namespace QP

//****************************************************************************
// NOTE01:
// QF::publish() increments the reference counter to prevent premature
// recycling of the event while the multicasting is still in progress.
// At the end of the function, the garbage collector step decrements the
// reference counter and recycles the event if the counter drops to zero.
// This covers the case when the event was published without any subscribers.
//
