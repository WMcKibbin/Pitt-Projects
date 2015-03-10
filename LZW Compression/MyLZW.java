/*************************************************************************
 *  Compilation:  javac LZW.java
 *  Execution:    java LZW - < input.txt   (compress)
 *  Execution:    java LZW + < input.txt   (expand)
 *  Dependencies: BinaryIn.java BinaryOut.java
 *
 *  Compress or expand binary input from standard input using LZW.
 *
 *  WARNING: STARTING WITH ORACLE JAVA 6, UPDATE 7 the SUBSTRING
 *  METHOD TAKES TIME AND SPACE LINEAR IN THE SIZE OF THE EXTRACTED
 *  SUBSTRING (INSTEAD OF CONSTANT SPACE AND TIME AS IN EARLIER
 *  IMPLEMENTATIONS).
 *
 *  See <a href = "http://java-performance.info/changes-to-string-java-1-7-0_06/">this article</a>
 *  for more details.
 *
 *************************************************************************/
//Author: William McKibbin
    //Adapted from: Algorithms 4th ed.  Sedgewick, Wayne
    //ISBN-10: 0-321-57351-X
//Intructor: Dr. Farnan CS 1501
//Date: 2/10/15
import java.lang.Math;
import java.io.*;
public class MyLZW {
    private static int R = 256;        // number of input chars
    private static int L = 512;       // number of codewords = 2^W
    private static int W = 9;         // codeword width
    private static char M;             // Compression Mode
	private static int MAXCODES;
	
	//Monitor mode vars
	private static double curRatio = 0;
	private static double orgRatio = 0;
	private static double expanBits = 0;//expanded bits
	private static double compBits = 0;//compressed bits
    /*********************************************************************/

    public static void compress() { 
        
        //Update bit length and max codeword size
		
		String input = BinaryStdIn.readString();
        TST<Integer> st = new TST<Integer>();
        for (int i = 0; i < R; i++)
        {
            st.put("" + (char) i, i);
        }
        int code = R+1;  // R is codeword for EOF
		
        while (input.length() > 0) {
			
			String s = st.longestPrefixOf(input);  // Find max prefix match s.
			BinaryStdOut.write(st.get(s), W);      // Print s's encoding.
            if(M == 'm')
			{
				expanBits += s.length() * 8;
				compBits += W;
				curRatio = expanBits / compBits;
				
				//System.err.print("Calculating ratio:   " + curRatio + "\n");
				//System.err.print("Orginal ratio:   " + orgRatio + "\n");
			}
            int t = s.length();           
            if (t < input.length() && code < L)    // Add s to symbol table.
            {
                st.put(input.substring(0, t + 1), code++);
            }
            if((code == L) && (W < 16))
            {
                W += 1;
                L = L * 2;
				st.put(input.substring(0, t + 1), code++);
            }
            if(code == MAXCODES)// || (orgRatio > 0))//reset codebook
            {
				if(M =='r')
				{	
					
					W = 9;
					L = 512;
					st = new TST<Integer>();
					for (int i = 0; i < R; i++)
					{
						st.put("" + (char) i, i);
					}
					code = R+1;
				}
				else if(M == 'm')
				{
					if(orgRatio == 0)
					{
						orgRatio = curRatio;
					}
					else if(orgRatio/curRatio > 1.1)
					{
						
						W = 9;
						L = 512;
						st = new TST<Integer>();
						for (int i = 0; i < R; i++)
						{
							st.put("" + (char) i, i);
						}
						code = R+1;
					}
				}
			}
            input = input.substring(t);            // Scan past s in input.
        }
        BinaryStdOut.write(R, W);
        BinaryStdOut.close();
    }

    /*********************************************************************/
    public static void expand() throws IOException{
        String[] st = new String[(int)Math.pow(2, 16)];
        int i; // next available codeword value
        
        M = BinaryStdIn.readChar(8);  //get the compression mode from the file
        // initialize symbol table with all 1-character strings
        for (i = 0; i < R; i++)
        {
            st[i] = "" + (char) i;
        }
        st[i++] = "";                        // (unused) lookahead for EOF
		
        int codeword = BinaryStdIn.readInt(W);
		compBits = W;
        if (codeword == R)return;           // expanded message is empty string
        String val = st[codeword];
        while (true) 
        {
			
            BinaryStdOut.write(val);
			//Compression ratio calculations
			expanBits = expanBits + val.length() * 8;
			codeword = BinaryStdIn.readInt(W);
			compBits = compBits + W;
			curRatio = expanBits / compBits;
			
			
			if (codeword == R)break;
            String s = st[codeword];
            if (i == codeword) s = val + val.charAt(0);   // special case hack
            if (i < L-1)
            {   
                st[i++] = val + s.charAt(0);
            }
            if((i == (L-1)) && (W < 16))//check to see of current width codewords are maxed out
            {
                W += 1;
                L = L * 2;
				st[i++] = val + s.charAt(0);
            }
            val = s;
			if(i == (MAXCODES - 1))// || (orgRatio > 0))//check to see if 16 bit codes are maxed out
            {
                if(M == 'r') //reset  if it is in reset mode
                {
					//reset bit width and code word limit
                    W = 9;
                    L = 512;
                    //reinitialize symbol table
                    st = new String[(int)Math.pow(2, 16)];
                    for (i = 0; i < R; i++)
                    {
                        st[i] = "" + (char) i;
                    }
                    st[i++] = "";
					//write out the last code and reset val
					BinaryStdOut.write(val);
					codeword = BinaryStdIn.readInt(W);
					if (codeword == R) return;
					val = st[codeword]; 
                }
				else if(M == 'm')
				{
					if(orgRatio == 0)
					{
						orgRatio = curRatio;
					}
					else if(orgRatio/curRatio > 1.1)
					{
						
						//reset bit width and code word limit
						W = 9;
						L = 512;
						//reinitialize symbol table
						st = new String[(int)Math.pow(2, 16)];
						for (i = 0; i < R; i++)
						{
							st[i] = "" + (char) i;
						}
						st[i++] = "";
						//write out the last code and reset val
						BinaryStdOut.write(val);
						expanBits = expanBits + (val.length() * 8);
						codeword = BinaryStdIn.readInt(W);
						compBits = compBits + W;
						if (codeword == R) return;
						val = st[codeword];
						
						
					}
				}
            }
			
            
        }
		
        BinaryStdOut.close();
    }
    
    /*********************************************************************/

    public static void main(String[] args) throws IOException
    {
        MAXCODES = (int)Math.pow(2,16);
        if(args[0].equals("-"))
        {
            if(args[1].equals("n"))
               M = 'n';
            else if(args[1].equals("r"))
                M = 'r';
            else if(args[1].equals("m"))
                M = 'm';
            else
                throw new IllegalArgumentException("Illegal command line argument");

            switch(M)
            {
                case 'n'://Variable length from 9 to 16bits
					BinaryStdOut.write('n', 8);
                    compress();
                    break;
                case 'm'://Variable length from 9 to 16bits with codebooks resets based on compression ratio
					BinaryStdOut.write('m', 8);
                    compress();
                    break;
                case 'r'://Variable length from 9 to 16bits resets after all 16bit codewords are used
					BinaryStdOut.write('r', 8);
                    compress();
                    break;
            }
        }
        else if (args[0].equals("+"))
        {
            expand();
        }
        else throw new IllegalArgumentException("Illegal command line argument");
    }
   
}