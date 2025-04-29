#pragma once
#include <cstdarg>
namespace Eloquent {
    namespace ML {
        namespace Port {
            class DecisionTree {
                public:
                    /**
                    * Predict class for features vector
                    */
                    int predict(float *x) {
                        if (x[4] <= 1.5) {
                            return 0;
                        }

                        else {
                            if (x[5] <= 3.0) {
                                return 1;
                            }

                            else {
                                return 2;
                            }
                        }
                    }

                protected:
                };
            }
        }
    }