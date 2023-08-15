#pragma once
#ifndef _SHARPEN_CONSENSUSCONFIGRESULT_HPP
#define _SHARPEN_CONSENSUSCONFIGRESULT_HPP

namespace sharpen {
    enum class ConsensusConfigResult {
        Changed,
        Invalid,
        Initialized
    };
}

#endif