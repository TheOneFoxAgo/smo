#ifndef RETURN_CODES_H_
#define RETURN_CODES_H_

namespace codes {
enum Result {
  success = 0,
  invalidArguments = 1,
  outputFileError = 2,
  configError = 3,
  incorrectGuess = 4,
};
}

#endif
