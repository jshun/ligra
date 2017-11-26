#include <iostream>
#include <fstream>

using namespace std;

int main() {
	ofstream myfile("output.txt");

	if(myfile.is_open()){
		for(long i=0; i < 256; i++){
			myfile << "case " << i << ": " << endl;
			myfile << "{" << endl;
			//myfile << "\t num_bytes = " << (i & 0x3) + 1 << ";" << endl;
			myfile << "\t edgeRead = 0;" << endl;
			myfile << "\t memcpy(&edgeRead, &start[dOffset], "<<(i&0x3)+1<<");" << endl;
			myfile << "\t dOffset += "<<(i&0x3)+1<<";" << endl;
			//myfile << "\t (*dOffsetPtr) += num_bytes;" << endl;
			myfile << "\t edge = startEdge + edgeRead;" << endl;
			myfile << "\t startEdge = edge;" << endl;
			myfile << "\t edgesRead++;" << endl;
			myfile << "\t if (!t.srcTarg(source, edge, edgesRead)) return 1;"<<endl;
			//myfile << "\t \t return 1;" << endl;
			//myfile << "\t \t }" << endl;
		
			//myfile << "\t num_bytes = " << ((i >> 2) & 0x3) + 1 << ";" << endl;
			myfile << "\t edgeRead = 0;" << endl;
			myfile << "\t memcpy(&edgeRead, &start[dOffset], "<<((i>>2)&0x3)+1<<");" << endl;
			myfile << "\t dOffset += "<<((i>>2)&0x3)+1<<";" << endl;
			//myfile << "\t (*dOffsetPtr) += num_bytes;" << endl;
			myfile << "\t edge = startEdge + edgeRead;" << endl;
			myfile << "\t startEdge = edge;" << endl;
			myfile << "\t edgesRead++;" << endl;
			myfile << "\t if (!t.srcTarg(source, edge, edgesRead)) return 1;"<<endl;
			//myfile << "\t \t return 1;" << endl;
			//myfile << "\t \t }" << endl;

			//myfile << "\t num_bytes = " << ((i >> 4) & 0x3) + 1 << ";" << endl;
			myfile << "\t edgeRead = 0;" << endl;
			myfile << "\t memcpy(&edgeRead, &start[dOffset], "<<((i>>4)&0x3)+1<<");" << endl;
			myfile << "\t dOffset += "<<((i>>4)&0x3)+1<<";" << endl;
			//myfile << "\t (*dOffsetPtr) += num_bytes;" << endl;
			myfile << "\t edge = startEdge + edgeRead;" << endl;
			myfile << "\t startEdge = edge;" << endl;
			myfile << "\t edgesRead++;" << endl;
			myfile << "\t if (!t.srcTarg(source, edge, edgesRead)) return 1;"<<endl;
			//myfile << "\t \t return 1;" << endl;
			//myfile << "\t \t }" << endl;
			
			//myfile << "\t num_bytes = " << ((i >> 6) & 0x3) + 1 << ";" << endl;
			myfile << "\t edgeRead = 0;" << endl;
			myfile << "\t memcpy(&edgeRead, &start[dOffset], "<<((i>>6)&0x3)+1<<");" << endl;
			//myfile << "\t (*dOffsetPtr) += num_bytes;" << endl;
			myfile << "\t dOffset += "<<((i>>6)&0x3)+1<<";" << endl;
			myfile << "\t edge = startEdge + edgeRead;" << endl;
			myfile << "\t startEdge = edge;" << endl;
			myfile << "\t edgesRead++;" << endl;
			myfile << "\t if (!t.srcTarg(source, edge, edgesRead)) return 1;"<<endl;
			//myfile << "\t \t return 1;" << endl;
			//myfile << "\t \t }" << endl;
			myfile << "\t break;" << endl;	
			myfile << "}" << endl;
		}
	}

	return 0;
}
