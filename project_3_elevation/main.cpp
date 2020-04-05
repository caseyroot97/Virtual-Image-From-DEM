//  ECE 1305 Project 03 - Digital Elevation Map to PGM
//  Program to create a virtual image from a Digital Elevation Map
//
//  main.cpp
//  project_3_elevation
//
//  Created by Casey Root on 12/01/17.
//  Copyright © 2017 Casey Root. All rights reserved.
//

#define _USE_MATH_DEFINES
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>
#include <cmath>
using namespace std;

string base_folder = "/Users/caseyroot/Desktop/DEMs/";

struct egm // Struct for header info
{
    char type[2]; // Value for the file type
    int nr; // Value for the number of rows
    int nc; // Value for the number of columns
    double minel; // Value for the minimum elevation
    double maxel; // Value for the maximum elevation
    double xscal; // Value for the x scalar
    double yscal; // Value for the y scalar
};
struct Vec3d // Struct for 3D info
{
    double x, y, z;
};
struct mars // Struct for mars coordinate info
{
    double lat=-1;
    double lon=-1;
    double degh=-1;
    double degw=-1;
    char n;
};

void get_filenames(string &qname,string &fn,string &outname,mars &read); // A function to prompt for and get input and output filenames, and optionally a lat/lon region to extract
bool read_EGM(bool open,string fn,egm &read,double *&pix); // Returns true if able to read, false if not
void rescale_EGM_to_PGM(string qname,string &outname,egm rescale,double *pix,short int *&newpix,int &maxval); // Function prototype to rescale the egm to pgm format
Vec3d get_sun_angle(void); // Asks for sun angle in az & el, returns it as a Vec3d
void illuminate_PGM_from_EGM(egm ill,Vec3d ray,double *pix,short int *&newpix,int &maxval); // Function prototype to rescale and illuminate the egm to pgm format
double dot_product(Vec3d sun,Vec3d norm); // Function prototype to calculate the dot product
Vec3d cross_product(Vec3d x,Vec3d y); // Function prototype to calculate the cross product
double mag(Vec3d v); // Function prototype to calculate the magnitude
Vec3d unit_vector(Vec3d v); // Function prototype to calculate the unit vector
void write_pgm(string qname,string fn,string outname,egm out,short int *newpix,int maxval,mars write); // Writes out image to PGM file

int main(void)
{
    egm header; // Declares the header struct
    Vec3d sun; // Declares vector struct for the sun's angle
    mars coor; // Declares the struct for mars coordinate info
    string name,fname, outname, resp; // Declares name and response variables
    bool open=false; // Declares bool variable
    int maxvalue; // Declares the maxvalue variable
    short int *newpixels; // Declares the pixel pointers
    double *pixels; // Declares the variables for the complete Mars elevation map
    get_filenames(name,fname,outname,coor); // Gets the input and output filenames
    open = read_EGM(open,fname,header,pixels); // Reads in the elevation map
    if (open==false) // Verifies the file was read
    {
        cout << "Error: couldn't read file " << fname << endl;
        return 1;
    }
    //cout << "Type: "<< header.type[0] << header.type[1] << endl << "Number Columns, Rows: " <<  header.nc  << ", " << header.nr  << endl << "Min,Max Elevation: " << header.minel  << ", " << header.maxel  << endl << "X,Y Scaling: " << header.xscal  << ", " << header.yscal << endl; // Displays header info for debugging
    cout << "Simple rescale?(Y/N) "; // Prompts to see if the user just wants to rescale
    cin >> resp; // Reads in the response
    if (resp == "y" or resp == "Y") // Statement for if the user just wants to rescale
        rescale_EGM_to_PGM(name,outname,header,pixels,newpixels,maxvalue); // Rescales the EGM elevations to values between 0 & 255
    else // Statement for if the user wants to illuminate the image
    {
        sun = get_sun_angle(); // Prompts for the sun angle
        illuminate_PGM_from_EGM(header,sun,pixels,newpixels,maxvalue); // Illuminates the EGM
    }
    write_pgm(name,fname,outname,header,newpixels,maxvalue,coor); // Writes the output image to a PGM file
    system(("open "+outname).c_str()); // Opens the new pgm image
    return 0; // Ends function main
}

void get_filenames(string &qname,string &fn,string &outname,mars &read) // Prompts for a filename and returns a filename, also makes sure we can open the file, and bails if we can't and the user decides to quit
{
    string prompt = "Enter EGM filename, with NO extension (or q to quit): "; // Prompts for the filename
    bool ok = false; // Declares and initializes bool variable
    while (!ok) // Checks to see if we can open the file
    {
        cout << prompt; // Outputs the prompt
        cin >> qname; // Reads in the filename
        fn = base_folder + qname + ".egm"; // Changes the filename
        ifstream ifs(fn); // Declares input file stream
        if (!ifs) // Verifies the file stream could be openned
        {
            if (qname == "q" || qname == "Q")
                exit(1); // Bails out if user wants to quit
            prompt = "Error opening "+fn+"\nEnter EGM filename, with NO extension (or q to quit): ";
        }
        else // Changes the bool variable to true to verify the file stream was opened successfully
        {
            ok = true; // We're good, we can open the file for reading
            ifs.close(); // Closes the file stream
        }
    }
    outname = base_folder+qname+"_illum.pgm"; // Changes the filename
    if (!strncmp(qname.c_str(),"mola",4)) // If EGM is complete Mars elevation map, gets lat, lon & box size of region to extract
    {
        cout << "File is complete Mars EGM " << qname << "\nOnly use low res mola" << endl; // Displays EGM is a complete Mars elevation map
        while (read.lat<0||read.lat>90)
        {
            cout << "What's your center point latitude?(Leave out North/South) "; // Prompts for the latitude of a location
            cin >> read.lat; // Reads in the latitude
            if (read.lat<0||read.lat>90) // Verifies latitude is within range
            {
                cout << "Error: please enter a latitude between 0 and 90!" << endl << "What's your center point latitude?(Leave out North/South) "; // Prompts for the latitude of a location
                cin >> read.lat; // Reads in the latitude
            }
        }
        cout << "Is your latitude north or south?(N/S) "; // Prompts for latitude direction
        cin >> read.n; // Reads in north or south
        while (read.lon<0||read.lon>360)
        {
            cout << "What's your center point longitude?(Leave out East/West) "; // Prompts for the longitude of a location
            cin >> read.lon; // Reads in the longitude
            if (read.lon<0||read.lon>360) // Verifies longitude is within range
            {
                cout << "Error: please enter a longitude between 0 and 360!" << endl << "What's your center point longitude?(Leave out East/West) "; // Prompts for the latitude of a location
                cin >> read.lon; // Reads in the latitude
            }
        }
        while (read.degh<0||read.degh>180)
        {
            cout << "How many degrees tall do you want your area? "; // Prompts for the height of a location in degrees the user wants to view
            cin >> read.degh; // Reads in the height degrees
            if (read.degh<0||read.degh>180) // Verifies height is within range
            {
                cout << "Error: please enter degrees between 0 and 180!" << endl << "How many degrees tall do you want your area? "; // Prompts for the latitude of a location
                cin >> read.degh; // Reads in the latitude
            }
        }
        while (read.degw<0||read.degw>360)
        {
            cout << "How many degrees wide do you want your area? "; // Prompts for the width of a location in degrees the user wants to view
            //read.degw = 60;
            cin >> read.degw; // Reads in the width degrees
            if (read.degw<0||read.degw>360) // Verifies width is within range
            {
                cout << "Error: please enter degrees between 0 and 360!" << endl << "How many degrees wide do you want your area? "; // Prompts for the latitude of a location
                cin >> read.degw; // Reads in the latitude
            }
        }
    }
}

bool read_EGM(bool open,string fn,egm &read,double *&pix) // Returns true if able to read, false if not
{
    int sizeinfo[6]; // Declares the array to read all the header info from the egm
    string type; // Declares type variable
    ifstream ifs(fn); // Creates an input file stream
    if (!ifs) // Verifies ifs opened successfully
    {
        cerr << "Error: couldn't open ifs!" << endl;
    }
    ifs >> type;
    ifs.close(); // Closes the input file stream
    if (type=="E4") // Verifys the correct file type
    {
        ifstream ifs(fn,ios::binary); // Creates an input file stream
        ifs.read(read.type,2); // Reads in the file type
        char junk; // Declares junk variable
        ifs.read(&junk,6); // Reads spaces after header info
        ifs.read(reinterpret_cast<char *>(sizeinfo),6*sizeof(int)); // Reads in the rest of the header info
        read.nc = sizeinfo[0]; // Assigns the number of columns to the struct
        read.nr = sizeinfo[1]; // Assigns the number of rows to the struct
        read.minel = sizeinfo[2]; // Assigns the minimum elevation to the struct
        read.maxel = sizeinfo[3]; // Assigns the maximum elevation to the struct
        read.xscal = sizeinfo[4]; // Assigns the x scalar to the struct
        read.yscal = sizeinfo[5]; // Assigns the y scalar to the struct
        int size = read.nc * read.nr; // Declares and calculates the amount of numbers
        pix = new double[size];
        short int *pix_int = new short int[size]; // Assigns pointer pix to an array the length of size
        ifs.read(reinterpret_cast<char *>(pix_int),size*sizeof(short int)); // Reads all the pixel values into the pixel array pointer
        ifs.close(); // Closes the input file stream
        open = true;
        for (int i=0;i<size;i++) // Loop to convert the pixels to pgm format
        {
            double pixel = pix_int[i];
            pix[i] = pixel;
        }
    }
    else if (type=="E1") // Verifys the correct file type
    {
        ifstream ifs(fn); // Declares and opens an input filestream
        ifs >> read.type[0] >> read.type[1] >> read.nc >> read.nr >> read.minel >> read.maxel >> read.xscal >> read.yscal; // Reads all the header values
        int size = read.nc * read.nr; // Declares and calculates the amount of numbers
        pix = new double[size]; // Assigns pointer pix to an array the length of size
        cout << read.type[0] << read.type[1] << " " << read.nc << " " << read.nr << " " << read.minel << " " << read.maxel << " " << read.xscal << " " << read.yscal; // Displays all the header values for debugging
        for (int i=0; i<size; i++) // Loop to fill the pix array with all the values in the PGM
        {
            ifs >> pix[i];
            //cout << pix[i] << " "; // Displays the array for debugging
        }
        cout << endl; // Creates a new line
        ifs.close(); // Closes the file
        open = true;
    }
    else // Returns message if right file type isn't detected
    {
        cerr << "Error: wrong file type!" << endl;
    }
    return open; // Returns the bool variable
}

void rescale_EGM_to_PGM(string qname,string &outname,egm rescale,double *pix,short int *&newpix,int &maxval) // Function to rescale the egm to pgm format
{
    int size = rescale.nc*rescale.nr; // Creates the variable size for the total number of pixels
    newpix = new short int[size]; // Creates the pixel array pointer for the converted pixels
    outname = base_folder + qname + ".pgm";
    //cout << "Type: "<< rescale.type[0] << rescale.type[1] << endl << "Number Columns, Rows: " <<  rescale.nc  << ", " << rescale.nr  << endl << "Min,Max Elevation: " << rescale.minel  << ", " << rescale.maxel  << endl << "X,Y Scaling: " << rescale.xscal  << ", " << rescale.yscal << endl; // Displays header info for debugging
    cout << "Converting to PGM..." << endl; // Displays converting has began
    for (int i=0;i<size;i++) // Loop to convert the pixels to pgm format
    {
        newpix[i] = (pix[i]+abs(rescale.minel))/(rescale.maxel+abs(rescale.minel))*255;
        //cout << newpix[i] << " "; // Displays new pixel array for debugging
        if (i == rescale.nc*rescale.nr/4) // Displays percentage converting complete
        {
            cout << "25% converting..." << endl;
        }
        if (i == rescale.nc*rescale.nr/2) // Displays percentage converting complete
        {
            cout << "50% converting..." << endl;
        }
        if (i == 3*rescale.nc*rescale.nr/4) // Displays percentage converting complete
        {
            cout << "75% converting..." << endl;
        }
    }
    cout << "Done converting!" << endl; // Displays converting complete
    delete []pix; // Deletes pix array
    maxval = (rescale.maxel+abs(rescale.minel))/(rescale.maxel+abs(rescale.minel))*255; // Converts the maximum pixel value to pgm format
}

Vec3d get_sun_angle(void) // Gets direction to sun and converts it to a 3-D vector
{
    double az, el; // Declares azimuth and elevation variables
    cout << "Sun Azimuth?(deg): "; // Prompts for azimuth
    cin >> az; // Reads in azimuth
    az = az*180.0/3.14159265; // Calculates azimuth
    cout << "Sun Elevation?(deg) "; // Prompts for elevation
    cin >> el; // Reads in elevation
    el = el*180.0/3.14159265; // Calculates elevation
    Vec3d sun; // Declares sun struct
    sun.x = cos(el) * cos(az); // Assigns value to sun struct
    sun.y = cos(el) * sin(az); // Assigns value to sun struct
    sun.z = sin(el); // Assigns value to sun struct
    return sun; // Returns sun struct
}

void illuminate_PGM_from_EGM(egm ill,Vec3d ray,double *pix,short int *&newpix,int &maxval) // Function to rescale and illuminate the egm to pgm format
{
    double angle; // Declares angle variable
    Vec3d normal,x,y; // Declares structs
    int size = ill.nc*ill.nr; // Creates the variable size for the total number of pixels
    newpix = new short int[size]; // Creates the pixel array pointer for the converted pixels
    maxval = 0; // Assigns 0 value to maxval
    cout << "Converting to PGM..." << endl; // Displays converting has began
    for (int i=0;i<size;i++)
    {
        if (i+1 % ill.nc == 0) // When the array is being calculated along the right edge
        {
            x.x = ill.xscal;
            x.y = 0;
            x.z = pix[i]-pix[i-1];
            
            y.x = 0;
            y.y = ill.yscal;
            y.z = pix[i+ill.nc]-pix[i];
        }
        else if (i+1 > size-ill.nc) // When the array is being calculated along the bottom edge
        {
            x.x = ill.xscal;
            x.y = 0;
            if (i == ill.nc*ill.nr-1)
            {
                x.z = pix[i-1]-pix[i];
            }
            else
            {
                x.z = pix[i+1]-pix[i];
            }
            
            y.x = 0;
            y.y = ill.yscal;
            y.z = pix[i]-pix[i-ill.nc];
        }
        else // When the rest of the array is being calculated
        {
            x.x = ill.xscal;
            x.y = 0;
            x.z = pix[i+1]-pix[i];
            
            y.x = 0;
            y.y = ill.yscal;
            y.z = pix[i+ill.nc]-pix[i];
        }
        normal = cross_product(x,y);
        angle = dot_product(normal,ray);
        newpix[i] = 255*(0.9*abs(angle)+0.1);
        if (newpix[i]>maxval)
        {
            maxval = newpix[i];
        }
        if (i == ill.nc*ill.nr/4) // Displays percentage converting complete
        {
            cout << "25% converting..." << endl;
        }
        if (i == ill.nc*ill.nr/2) // Displays percentage converting complete
        {
            cout << "50% converting..." << endl;
        }
        if (i == 3*ill.nc*ill.nr/4) // Displays percentage converting complete
        {
            cout << "75% converting..." << endl;
        }
    }
    cout << "Done converting!" << endl; // Displays converting complete
}

Vec3d cross_product(Vec3d x,Vec3d y) // Function to calculate the cross product
{
    Vec3d norm;
    norm.x = x.y*y.z-y.y*x.z;
    norm.y = x.z*y.x-y.z*x.x;
    norm.z = x.x*y.y-y.x*x.y;
    return norm;
}

double dot_product(Vec3d norm,Vec3d sun) // Function to calculate the dot product
{
    double ang;
    norm = unit_vector(norm);
    sun = unit_vector(sun);
    ang = norm.x*sun.x + norm.y*sun.y + norm.z*sun.z;
    return ang;
}

Vec3d unit_vector(Vec3d v) // Function to calculate the unit vector
{
    double mymag = mag(v);
    v.x /= mymag;
    v.y /= mymag;
    v.z /= mymag;
    return v;
}

double mag(Vec3d v) // Function to calculate the magnitude
{
    return sqrt(v.x*v.x + v.y*v.y + v.z * v.z);
}

void write_pgm(string qname,string fn,string outname,egm out,short int *newpix,int maxval,mars write) // Function to write out the whole pgm file
{
    if (!strncmp(qname.c_str(),"mola",4))
    {
        int x,y,x_beg,x_end,y_beg,y_end;
        if (write.n == 'n' || write.n == 'N') // Calculates the y value if the latitude is north
        {
            y = 90*16-write.lat*16;
        }
        else // Calculates the y value if the latitude is south
        {
            y = 90*16+write.lat*16;
        }
        x = 360*16-write.lon*16; // Calculates the x value
        x_beg = x-write.degw/2*16; // Calculates the x value for the beginning of the area box
        x_end = x+write.degw/2*16; // Calculates the x value for the end of the area box
        y_beg = y-write.degh/2*16; // Calculates the y value for the beginning of the area box
        y_end = y+write.degh/2*16; // Calculates the y value for the end of the area box
        int col = x_end - x_beg; // Creates the new number of columns value
        int row = y_end - y_beg; // Creates the new number of rows value
        cout << "x,y: " << x << ", " << y << endl << "x_beg, x_end: " << x_beg << ", " << x_end << endl << "y_beg, y_end: " << y_beg << ", " << y_end << endl << "Columns,Rows: " << col << ", " << row << endl << "Maxval: " << maxval << endl; // Displays variables for debugging
        ofstream ofs(outname); // Creates a output file stream and a new file
        cout << "Writing Mars feature PGM..."; // Displays that the new pgm is in the process of being written
        short int *geopix = new short int[col*row]; // Creates the new pixel array pointer
        int a = 0,b; // Declares variables for writing out the new pgm
        for (int i=y_beg;i<y_end;i++) // Loop for writing out the new pgm in the y direction
        {
            for (int j=x_beg;j<x_end;j++) // Loop for writing out the new pgm in the x direction
            {
                b = (i-1)*out.nc+j; // Calculates the element value for the new pixel array
                geopix[a] = newpix[b]; // Sets the geographic pixel array equal to the new pixel array value
                //cout << b << " "; // Displays array element for debugging
                a++; // Raises the geographic pixel array element for each loop
            }
        }
        cout << endl; // Creates a new line
        delete []newpix;
        //cout << "x,y: " << x << ", " << y << endl << "x_beg, x_end: " << x_beg << ", " << x_end << endl << "y_beg, y_end: " << y_beg << ", " << y_end << endl << "Columns,Rows: " << col << ", " << row << endl << "Maxval: " << maxval << endl; // Displays variables for debugging
        ofs << "P2" << " " << col << " " << row << " " << maxval << endl; // Writes the data saved to the header variables into the new file
        cout << "P2" << " " << col << " " << row << " " << maxval << endl;
        for(int i=0;i<col*row;i++) // Loop to write out all the the geographic pixel array into the new pgm file
        {
            ofs << geopix[i] << " ";
            if (i == col*row/4)
            {
                cout << "25% done writing..." << endl;
            }
            if (i == col*row/2)
            {
                cout << "50% done writing..." << endl;
            }
            if (i == 3*col*row/4)
            {
                cout << "75% done writing..." << endl;
            }
        }
        cout << "Done writing!" << endl;
        ofs.close(); // Closes the output file stream
        delete []geopix;
    }
    else
    {
        ofstream ofs(outname); // Creates a output file stream and a new file
        cout << "Writing PGM..." << endl; // Displays that the pgm is in the process of being written
        ofs << "P2" << " " << int(out.nc) << " " << int(out.nr) << " " << maxval << endl; // Writes the data saved to the header variables into the new file
        for (int i=0;i<out.nc*out.nr;i++) // Loop to write out the pgm
        {
            ofs << newpix[i] << " ";
            if (i == out.nc*out.nr/4)
            {
                cout << "25% done writing..." << endl;
            }
            if (i == out.nc*out.nr/2)
            {
                cout << "50% done writing..." << endl;
            }
            if (i == 3*out.nc*out.nr/4)
            {
                cout << "75% done writing..." << endl;
            }
        }
        cout << "Done writing!" << endl;
        ofs.close(); // Closes the output file stream
        delete []newpix; // Cleans up dynamically allocated memory
    }
}
