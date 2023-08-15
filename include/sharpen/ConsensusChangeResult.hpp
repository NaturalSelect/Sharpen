#pragma once
#ifndef _SHARPEN_CONSENSUSCONFIGRESULT_HPP
#define _SHARPEN_CONSENSUSCONFIGRESULT_HPP

namespace sharpen {
    enum class ConsensusChangeResult
    {
        // operation success
        Success,
        
        // only for ConfiguratePeer:
        // means that new peers set is invalid
        Invalid,

        // for PreparePeersChanges:
        //      *   The config already be locked in prev logs, you need to invoke AbortPeersChanges to
        //          unlock it.
        // for AbortPeersChanges:
        //      *   The config already be unlocked in prev logs.
        Confilct
    };
}   // namespace sharpen

#endif