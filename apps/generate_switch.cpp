#include <iostream>
#include <fstream>

using namespace std;

/*

I used this file to generate the 256 case statement for streamcase

*/

void edge(int numBytes, ofstream &myfile) {
  if(numBytes == 1) {
    myfile << "\t edgeRead = start[dOffset++];" << endl;
  } else if(numBytes == 2) {
    myfile << "\t edgeRead = start[dOffset] + (start[dOffset+1] << 8);" << endl;
    myfile << "\t dOffset += 2;" << endl;
  } else if(numBytes == 3) {
    myfile << "\t edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);" << endl;
    myfile << "\t dOffset += 3;" << endl;
  } else {
    myfile << "\t edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);" << endl;
    myfile << "\t dOffset += 4;" << endl;
  }
}


int main() {
	ofstream myfile("output.txt");

	if(myfile.is_open()){
		for(long i=0; i < 256; i++){
			myfile << "case " << i << ": " << endl;
			myfile << "{" << endl;
			edge((i&3)+1,myfile);
			myfile << "\t edge = startEdge + edgeRead;" << endl;
			myfile << "\t startEdge = edge;" << endl;
			myfile << "\t edgesRead++;" << endl;
			myfile << "\t if (!t.srcTarg(source, edge, edgesRead)) return 1;"<<endl;
					
			edge(((i>>2)&3)+1,myfile);			
			myfile << "\t edge = startEdge + edgeRead;" << endl;
			myfile << "\t startEdge = edge;" << endl;
			myfile << "\t edgesRead++;" << endl;
			myfile << "\t if (!t.srcTarg(source, edge, edgesRead)) return 1;"<<endl;
			
			edge(((i>>4)&3)+1,myfile);
			myfile << "\t edge = startEdge + edgeRead;" << endl;
			myfile << "\t startEdge = edge;" << endl;
			myfile << "\t edgesRead++;" << endl;
			myfile << "\t if (!t.srcTarg(source, edge, edgesRead)) return 1;"<<endl;
			
			edge(((i>>6)&3)+1,myfile);
			myfile << "\t edge = startEdge + edgeRead;" << endl;
			myfile << "\t startEdge = edge;" << endl;
			myfile << "\t edgesRead++;" << endl;
			myfile << "\t if (!t.srcTarg(source, edge, edgesRead)) return 1;"<<endl;
			myfile << "\t break;" << endl;	
			myfile << "}" << endl;
		}
	}

	return 0;
}
