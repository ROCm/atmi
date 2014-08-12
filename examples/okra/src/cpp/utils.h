#include <fstream>
#include <stdlib.h>
using namespace std;

static char *buildStringFromSourceFile(string fname) {
	cout << "using source from " << fname << endl;
	ifstream infile;
	infile.open(fname.c_str());
	if (!infile) {cout << "could not open " << fname << endl; exit(1);}
	infile.seekg(0, ios::end);
	int len = infile.tellg();
	char *str = new char[len+1];
	infile.seekg(0, ios::beg);
	infile.read(str, len);
	int lenRead = infile.gcount();
	// if (!infile) {cout << "could not read " << len << " bytes from " << fname << " but read " << lenRead << endl;}
	str[lenRead] = (char)0;  // terminate
	// cout << "Source String -----------\n" << str << "----------- end of Source String\n";
	return str;
};
