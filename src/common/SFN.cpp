#include <iostream>
#include <string.h>

const int MEDIA_PRINT_FILENAME_SIZE = 128;
char l_fileName[MEDIA_PRINT_FILENAME_SIZE] = { 0 };

// -- try it yourself
// g++ -o ./SFN.cpp
// ./SFN

// Algorithm to convert long file names with paths to 8.3 DOS file names/paths
void LFN_TO_SFN(char *_sfn, char *_lfn) {
	char excluded[15] = { '"', '*', '+', ',', ':', ';', '<', '=', '>', '?'/*, '\', '[', ']', '|'*/ };
  int i = 0; // LFN char index
  int j = 0; // inner cycle loop index
  int k = 0; // generated SFN char index
	int l = 8; // 8 chars count
  int sl = strlen(_lfn);
  while (i < sl) {
    // start generating SFN when nesting begins
    if (_lfn[i] == '/' ||
        (i == 0 &&
         _lfn[i] != '/')) {

      // check end of the path
      bool ep = true;
      for (j = i+1; j < sl; j++) {
        if (_lfn[j] == '/') {
          ep = false;
          break;
        }
      }

      // if we are at the end we get index
      // of the last dot, if there is any
      int es = sl;
      if (ep) {
        for (j = sl; j > i; j--) {
          if (_lfn[j] == '.') {
            es = j;
            break;
          }
        }
      }

      l = 8;
      int ic = (i + 9) > (es - i) ? (es - i)
                                  : 9; // check for less or more chars for type
      for (j = 0; j < ic; j++) {  // count chars to generate SFN
        char aChar = _lfn[i + j]; // actual char
        char nChar = _lfn[i + j + 1]; // next char

        if (aChar == ' ' || aChar == '.') {
          ic += 1;
          continue;
        }

				// replace excluded chars
        int m = 0;
        for (m = 0; m < strlen(excluded); m++) {
          if (excluded[m] == _lfn[i + j]) {
            aChar = '_';
            break;
          }
        }

        // UPPERCASE char
        _sfn[k] = towupper(aChar);
        k++;
        l--;
        // when another nesting begins or filename ends - break the cycle
        if (nChar == '/' /*|| nChar == '.'*/) {
          break;
        }
        // name of path or file is too long - add numbers to the end
        // TODO - manage more files with same name !!
        if (l == 0) {
          _sfn[k - 2] = '~';
          _sfn[k - 1] = '1';
          break;
        }
      }
      i += j; // move LFN index to next part

      // the end is near!
      if (ep) {
        // file type
        for (j = es; j < es + 4; j++) {
          if (j <= sl) {
            _sfn[k] = towupper(_lfn[j]);
            k++;
          } else { break; }
        }
      }
    }
    // next index
    i++;
  }
}

int main() {
  // char *test_text = "/1/dvje/prusomaselnihovadina/ahojjako/se/mas/chleboslaF.gcode";
	// char *test_text = "masloslaf/prusalab/ahojjaksemas/TABLE_CORNER_HOLDER_0.25mm_PETG_MINI_12h18m.gcode";
  // strncpy(l_fileName, test_text, sizeof(l_fileName) - 1);
  char sfn[MEDIA_PRINT_FILENAME_SIZE] = {0};

  // <-- INPUT PATH TO CONVERT
  std::cout << "Please write your path to convert into a SFN:" << std::endl;
	std::cin.getline(l_fileName, MEDIA_PRINT_FILENAME_SIZE);
  LFN_TO_SFN(sfn, l_fileName);

  // --> OUTPUT 8.3 PATH
  std::cout << "--- LFN conversion to SFN ---" << std::endl;
  std::cout << "LFN - " << l_fileName << std::endl;
  std::cout << "SFN - " << sfn << std::endl;

  return 0;
}
