/***************************************************************************

  snesdsp1.c

  File to handle emulation of the SNES "DSP-1" add-on chip.

  Original C++ "dsp1emul.cpp" by Andreas Naive
  Based on research by Overload, The Dumper, Neviksti and Andreas Naive
  MAME/MESS C conversion by R. Belmont

  This is up to date with the source version dated June 2006.

***************************************************************************/

#define DSP1_VERSION 0x0102

// The DSP-1 status register has 16 bits, but only
// the upper 8 bits can be accessed from an external device, so all these
// positions are referred to the upper byte (bits D8 to D15)

enum SrFlags {DRC=0x04, DRS=0x10, RQM=0x80};

// According to Overload's docs, these are the meanings of the flags:
// DRC: The Data Register Control (DRC) bit specifies the data transfer length to and from the host CPU.
//   0: Data transfer to and from the DSP-1 is 16 bits.
//   1: Data transfer to and from the DSP-1 is 8 bits.
// DRS: The Data Register Status (DRS) bit indicates the data transfer status in the case of transfering 16-bit data.
//   0: Data transfer has terminated.
//   1: Data transfer in progress.
// RQM: The Request for Master (RQM) indicates that the DSP1 is requesting host CPU for data read/write.
//   0: Internal Data Register Transfer.
//   1: External Data Register Transfer.

enum FsmMajorState {WAIT_COMMAND, READ_DATA, WRITE_DATA};
enum MaxDataAccesses {MAX_READS=7, MAX_WRITES=1024};

struct DSP1_Command {
        void (*callback)(INT16 *, INT16 *);
        unsigned int reads;
        unsigned int writes;
};

static const struct DSP1_Command mCommandTable[0x40];
static const INT16 DSP1_MaxAZS_Exp[16];
static const INT16 DSP1_SinTable[256];
static const INT16 DSP1_MulTable[256];
static UINT16 DSP1_DataRom[1024];

static struct SharedData { // some RAM variables shared between commands
        INT16 MatrixA[3][3];          // attitude matrix A
        INT16 MatrixB[3][3];
        INT16 MatrixC[3][3];
        INT16 CentreX, CentreY, CentreZ;   // center of projection
        INT16 CentreZ_C, CentreZ_E;
        INT16 VOffset;                     // vertical offset of the screen with regard to the centre of projection
        INT16 Les, C_Les, E_Les;
        INT16 SinAas, CosAas;
        INT16 SinAzs, CosAzs;
        INT16 SinAZS, CosAZS;
        INT16 SecAZS_C1, SecAZS_E1;
        INT16 SecAZS_C2, SecAZS_E2;
        INT16 Nx, Ny, Nz;    // normal vector to the screen (norm 1, points toward the center of projection)
        INT16 Gx, Gy, Gz;    // center of the screen (global coordinates)
        INT16 Hx, Hy;        // horizontal vector of the screen (Hz=0, norm 1, points toward the right of the screen)
        INT16 Vx, Vy, Vz;    // vertical vector of the screen (norm 1, points toward the top of the screen)

} shared;

static UINT8 mSr;            // status register
static int mSrLowByteAccess;
static UINT16 mDr;           // "internal" representation of the data register
static enum FsmMajorState mFsmMajorState;     // current major state of the FSM
static UINT8 mCommand;                  // current command processed by the FSM
static UINT8 mDataCounter;                 // #UINT16 read/writes counter used by the FSM
static INT16 mReadBuffer[MAX_READS];
static INT16 mWriteBuffer[MAX_WRITES];
static UINT8 mFreeze;                   // need explanation?  ;)

static void DSP1_fsmStep(UINT8 read, UINT8 *data);            // FSM logic

// commands
static void DSP1_memoryTest(INT16 *input, INT16 *output);
static void DSP1_memoryDump(INT16 *input, INT16 *output);
static void DSP1_memorySize(INT16 *input, INT16 *output);
static void DSP1_multiply(INT16* input, INT16* output);
static void DSP1_multiply2(INT16* input, INT16* output);
static void DSP1_inverse(INT16 *input, INT16 *output);
static void DSP1_triangle(INT16 *input, INT16 *output);
static void DSP1_radius(INT16 *input, INT16 *output);
static void DSP1_range(INT16 *input, INT16 *output);
static void DSP1_range2(INT16 *input, INT16 *output);
static void DSP1_distance(INT16 *input, INT16 *output);
static void DSP1_rotate(INT16 *input, INT16 *output);
static void DSP1_polar(INT16 *input, INT16 *output);
static void DSP1_attitudeA(INT16 *input, INT16 *output);
static void DSP1_attitudeB(INT16 *input, INT16 *output);
static void DSP1_attitudeC(INT16 *input, INT16 *output);
static void DSP1_objectiveA(INT16 *input, INT16 *output);
static void DSP1_objectiveB(INT16 *input, INT16 *output);
static void DSP1_objectiveC(INT16 *input, INT16 *output);
static void DSP1_subjectiveA(INT16 *input, INT16 *output);
static void DSP1_subjectiveB(INT16 *input, INT16 *output);
static void DSP1_subjectiveC(INT16 *input, INT16 *output);
static void DSP1_scalarA(INT16 *input, INT16 *output);
static void DSP1_scalarB(INT16 *input, INT16 *output);
static void DSP1_scalarC(INT16 *input, INT16 *output);
static void DSP1_gyrate(INT16 *input, INT16 *output);
static void DSP1_parameter(INT16 *input, INT16 *output);
static void DSP1_raster(INT16 *input, INT16 *output);
static void DSP1_target(INT16 *input, INT16 *output);
static void DSP1_project(INT16 *input, INT16 *output);

// auxiliar functions
static INT16 DSP1_sin(INT16 Angle);
static INT16 DSP1_cos(INT16 Angle);
static void inverse(INT16 Coefficient, INT16 Exponent, INT16 *iCoefficient, INT16 *iExponent);
static INT16 denormalizeAndClip(INT16 C, INT16 E);
static void normalize(INT16 m, INT16 *Coefficient, INT16 *Exponent);
static void normalizeDouble(INT32 Product, INT16 *Coefficient, INT16 *Exponent);
static INT16 shiftR(INT16 C, INT16 E);

//////////////////////////////////////////////////////////////////

UINT8 DSP1_getSr(void)
{
   mSrLowByteAccess = ~mSrLowByteAccess;
   if (mSrLowByteAccess)
   {
      return 0;
   }
   else
   {
      return mSr;
   }
}

//////////////////////////////////////////////////////////////////

UINT8 DSP1_getDr(void)
{
   UINT8 oDr;

   DSP1_fsmStep(1, &oDr);
   return oDr;
}

//////////////////////////////////////////////////////////////////

void DSP1_setDr(UINT8 iDr)
{
    DSP1_fsmStep(0, &iDr);
}

//////////////////////////////////////////////////////////////////

void DSP1_reset(void)
{
	UINT32 i;
	UINT8 *dspin = memory_region(REGION_USER6);

	mSr = DRC|RQM;
	mSrLowByteAccess = FALSE;
	mDr = 0x0080;    // Only a supposition. Is this correct?
	mFreeze = FALSE;
	mFsmMajorState = WAIT_COMMAND;
	memset(&shared, 0, sizeof(struct SharedData)); // another supposition

	// expand the DSP-1 data ROM
	for (i = 0; i < 2048; i+=2)
	{
		DSP1_DataRom[i/2] = dspin[i]<<8 | dspin[i+1];
	}
}

//////////////////////////////////////////////////////////////////

// Though the DSP-1 is unaware of the type of operation (read or write)
// we need to know what is being done by the program, as the class
// is responsible for maintaining the binding between the
// "external" and "internal" representations of the DR (data register).

static void DSP1_fsmStep(UINT8 read, UINT8 *data)
{
   if (0 == (mSr&RQM)) return;
   // Now RQM would be cleared; however, as this code is not to be used in
   // a multithread environment, we will simply fake RQM operation.
   // (The only exception would be Op1A's freeze.)

   // binding
   if (read)
   {
      if (mSr&DRS)
         *data = (UINT8)(mDr>>8);
      else
         *data = (UINT8)(mDr);
   }
   else
   {
      if (mSr&DRS)
      {
         mDr &= 0x00ff;
         mDr |= *data<<8;
      }
      else
      {
         mDr &= 0xff00;
         mDr |= *data;
      }
   }


   switch (mFsmMajorState)
   {
      case WAIT_COMMAND:
         mCommand = (UINT8)(mDr);
         if (!(mCommand & 0xc0))   // valid command?
         {
            switch(mCommand)
            {
               // freeze cases
               case 0x1a:
               case 0x2a:
               case 0x3a:
                  mFreeze = TRUE;
                  break;
               // normal cases
               default:
                  mDataCounter=0;
                  mFsmMajorState = READ_DATA;
                  mSr &= ~DRC;
                  break;
            }
         }
         break;
      case READ_DATA:
         mSr ^= DRS;
         if (!(mSr&DRS))
         {
            mReadBuffer[mDataCounter++] = (INT16)(mDr);
            if (mDataCounter >= mCommandTable[mCommand].reads)
            {
               (*mCommandTable[mCommand].callback)(mReadBuffer, mWriteBuffer);
               if (0 != mCommandTable[mCommand].writes)  // any output?
               {
                  mDataCounter = 0;
                  mDr = (UINT16)(mWriteBuffer[mDataCounter]);
                  mFsmMajorState = WRITE_DATA;
               }
               else
               {
                  mDr = 0x0080;  // valid command completion
                  mFsmMajorState = WAIT_COMMAND;
                  mSr |= DRC;
               }
            }
         }
         break;
      case WRITE_DATA:
         mSr ^= DRS;
         if (!(mSr&DRS))
         {
            ++mDataCounter;
            if (mDataCounter >= mCommandTable[mCommand].writes)
            {
               if ((mCommand == 0x0a)&&(mDr != 0x8000))
               {
                  // works in continuous mode
                  mReadBuffer[0]++;   // next raster line
                  (*mCommandTable[mCommand].callback)(mReadBuffer, mWriteBuffer);
                  mDataCounter = 0;
                  mDr = (UINT16)(mWriteBuffer[mDataCounter]);
               }
               else
               {
                  mDr = 0x0080;  // valid command completion
                  mFsmMajorState = WAIT_COMMAND;
                  mSr |= DRC;
               }
            }
            else
            {
               mDr = (UINT16)(mWriteBuffer[mDataCounter]);
            }
         }
         break;
   }



   // Now RQM would be set (except when executing Op1A -command equals 0x1a, 0x2a or 0x3a-).
   if (mFreeze)
      mSr &= ~RQM;
}

//////////////////////////////////////////////////////////////////

// The info on this table follows Overload's docs.

static const struct DSP1_Command mCommandTable[0x40] =
{
   {&DSP1_multiply, 2, 1},   //0x00
   {&DSP1_attitudeA, 4, 0},    //0x01
   {&DSP1_parameter, 7, 4},   //0x02
   {&DSP1_subjectiveA, 3, 3},    //0x03
   {&DSP1_triangle, 2, 2},   //0x04
   {&DSP1_attitudeA, 4, 0},   //0x01
   {&DSP1_project, 3, 3},   //0x06
   {&DSP1_memoryTest, 1, 1},   //0x0f
   {&DSP1_radius, 3, 2},   //0x08
   {&DSP1_objectiveA, 3, 3},   //0x0d
   {&DSP1_raster, 1, 4},   // 0x0a. This will normally work in continuous mode
   {&DSP1_scalarA, 3, 1},   //0x0b
   {&DSP1_rotate, 3, 2},   //0x0c
   {&DSP1_objectiveA, 3, 3},   //0x0d
   {&DSP1_target, 2, 2},   //0x0e
   {&DSP1_memoryTest, 1, 1},   //0x0f

   {&DSP1_inverse, 2, 2},   //0x10
   {&DSP1_attitudeB, 4, 0},   //0x11
   {&DSP1_parameter, 7, 4},   //0x02
   {&DSP1_subjectiveB, 3, 3},   //0x13
   {&DSP1_gyrate, 6, 3},   //0x14
   {&DSP1_attitudeB, 4, 0},   //0x11
   {&DSP1_project, 3, 3},   //0x06
   {&DSP1_memoryDump, 1, 1024},   //0x1f
   {&DSP1_range, 4, 1},   //0x18
   {&DSP1_objectiveB, 3, 3},   //0x1d
   {0, 0, 0},   // 0x1a; the chip freezes
   {&DSP1_scalarB, 3, 1},   //0x1b
   {&DSP1_polar, 6, 3},   //0x1c
   {&DSP1_objectiveB, 3, 3},   //0x1d
   {&DSP1_target, 2, 2},   //0x0e
   {&DSP1_memoryDump, 1, 1024},   //0x1f

   {&DSP1_multiply2, 2, 1},   //0x20
   {&DSP1_attitudeC, 4, 0},   //0x21
   {&DSP1_parameter, 7, 4},   //0x02
   {&DSP1_subjectiveC, 3, 3},   //0x23
   {&DSP1_triangle, 2, 2},   //0x04
   {&DSP1_attitudeC, 4, 0},   //0x21
   {&DSP1_project, 3, 3},   //0x06
   {&DSP1_memorySize, 1, 1},    //0x2f
   {&DSP1_distance, 3, 1},   //0x28
   {&DSP1_objectiveC, 3, 3},   //0x2d
   {0, 0, 0},   // 0x1a; the chip freezes
   {&DSP1_scalarC, 3, 1},   //0x2b
   {&DSP1_rotate, 3, 2},   //0x0c
   {&DSP1_objectiveC, 3, 3},   //0x2d
   {&DSP1_target, 2, 2},   //0x0e
   {&DSP1_memorySize, 1, 1},   //0x2f

   {&DSP1_inverse, 2, 2},   //0x10
   {&DSP1_attitudeA, 4, 0},   //0x01
   {&DSP1_parameter, 7, 4},   //0x02
   {&DSP1_subjectiveA, 3, 3},   //0x03
   {&DSP1_gyrate, 6, 3},   //0x14
   {&DSP1_attitudeA, 4, 0},   //0x01
   {&DSP1_project, 3, 3},   //0x06
   {&DSP1_memoryDump, 1, 1024},   //0x1f
   {&DSP1_range2, 4, 1},   //0x38
   {&DSP1_objectiveA, 3, 3},   //0x0d
   {0, 0, 0},   // 0x1a; the chip freezes
   {&DSP1_scalarA, 3, 1},   //0x0b
   {&DSP1_polar, 6, 3},   //0x1c
   {&DSP1_objectiveA, 3, 3},   //0x0d
   {&DSP1_target, 2, 2},   //0x0e
   {&DSP1_memoryDump, 1, 1024},   //0x1f
};

//////////////////////////////////////////////////////////////////

static void DSP1_memoryTest(INT16 *input, INT16 *output)
{
//   INT16 *Size = &input[0];
   INT16 *Result = &output[0];

   *Result = 0x0000;
}

//////////////////////////////////////////////////////////////////

static void DSP1_memoryDump(INT16 *input, INT16 *output)
{
   memcpy(output, DSP1_DataRom, 1024);
}

//////////////////////////////////////////////////////////////////

static void DSP1_memorySize(INT16 *input, INT16 *output)
{
   INT16 *Size = &output[0];

   *Size = 0x0100;
}

//////////////////////////////////////////////////////////////////

// 16-bit multiplication

static void DSP1_multiply(INT16 *input, INT16 *output)
{
   INT16 Multiplicand = input[0];
   INT16 Multiplier = input[1];
   INT16 *Product = &output[0];

   *Product = Multiplicand * Multiplier >> 15;
}

//////////////////////////////////////////////////////////////////

// 16-bit multiplication. 'Alternative' method. Can anyone check this carefully?

static void DSP1_multiply2(INT16 *input, INT16 *output)
{
   INT16 Multiplicand = input[0];
   INT16 Multiplier = input[1];
   INT16* Product = &output[0];

   *Product = (Multiplicand * Multiplier >> 15)+1;
}

//////////////////////////////////////////////////////////////////

// This command determines the inverse of a floating point decimal number.

static void DSP1_inverse(INT16 *input, INT16 *output)
{
   INT16 Coefficient = input[0];
   INT16 Exponent = input[1];
   INT16* iCoefficient = &output[0];
   INT16* iExponent = &output[1];

   inverse(Coefficient, Exponent, iCoefficient, iExponent);
}

//////////////////////////////////////////////////////////////////

// Vector component calculation. Determines the X and Y components for a
// two-dimensional vector whose size and direction is known.
// Y = Radius * sin(Angle)
// X = Radius * cos(Angle)

static void DSP1_triangle(INT16 *input, INT16 *output)
{
   INT16 Angle = input[0];
   INT16 Radius = input[1];
   INT16* Y = &output[0];
   INT16* X = &output[1];

   *Y = DSP1_sin(Angle) * Radius >> 15;
   *X = DSP1_cos(Angle) * Radius >> 15;
}

//////////////////////////////////////////////////////////////////

// Determines the squared norm of a vector (X,Y,Z)
// The output is Radius = X^2+Y^2+Z^2 (double integer)

static void DSP1_radius(INT16 *input, INT16 *output)
{
   INT16 X = input[0];
   INT16 Y = input[1];
   INT16 Z = input[2];
   INT16* RadiusLow = &output[0];
   INT16* RadiusHigh = &output[1];

   INT32 Radius;

   Radius = (X * X + Y * Y + Z * Z) << 1;
   *RadiusLow = (INT16)(Radius);
   *RadiusHigh = (INT16)(Radius>>16);
}

//////////////////////////////////////////////////////////////////

// Vector size comparison. This command compares the size of the vector (X,Y,Z) and the distance (R)
// from a particular point, and so may be used to determine if a point is within the sphere or radius R.
// The output is D = X^2+Y^2+Z^2-R^2

static void DSP1_range(INT16 *input, INT16 *output)
{
   INT16 X = input[0];
   INT16 Y = input[1];
   INT16 Z = input[2];
   INT16 Radius = input[3];
   INT16* Range = &output[0];

   *Range = (X * X + Y * Y + Z * Z - Radius * Radius) >> 15;
}

//////////////////////////////////////////////////////////////////

// Vector size comparison. 'Alternative' method.

static void DSP1_range2(INT16 *input, INT16 *output)
{
   INT16 X = input[0];
   INT16 Y = input[1];
   INT16 Z = input[2];
   INT16 Radius = input[3];
   INT16* Range = &output[0];

   *Range = ((X * X + Y * Y + Z * Z - Radius * Radius) >> 15) + 1;
}

//////////////////////////////////////////////////////////////////

// This command calculates the norm of a (X,Y,Z) vector, or the distance from
// the point (X,Y,Z) to (0,0,0), as you prefer to see it.
// Distance = sqrt(X^2+Y^2+Z^2)
// The square root of a number 'a' is calculated by doing this: you
// write 'a' as b*2^2n, with 'b' between 1/4 and 1; then, you calculate
// c=sqrt(b) by using lineal interpolation between points of a
// look-up table and, finally, you output the result as c*2^n.

static void DSP1_distance(INT16 *input, INT16 *output)
{
   INT16 X = input[0];
   INT16 Y = input[1];
   INT16 Z = input[2];
   INT16* Distance = &output[0];
   INT16 Pos, Node1, Node2;

   INT32 Radius = X * X + Y * Y + Z * Z;


   if (Radius == 0) Distance = 0;
   else
   {
      INT16 C, E;
      normalizeDouble(Radius, &C, &E);
      if (E & 1) C = C * 0x4000 >> 15;

      Pos = C * 0x0040 >> 15;

      Node1 = DSP1_DataRom[0x00d5 + Pos];
      Node2 = DSP1_DataRom[0x00d6 + Pos];

      *Distance = ((Node2 - Node1) * (C & 0x1ff) >> 9) + Node1;

#if DSP1_VERSION < 0x0102
		if (Pos & 1) *Distance -= (Node2 - Node1);
#endif
		*Distance >>= (E >> 1);
   }
}

//////////////////////////////////////////////////////////////////

// Determines the (X2, Y2) coordinates obtained by rotating (X1, Y1)
// clockwise for an angle 'Angle'. The official documentation says
// 'counterclockwise', but it's obviously wrong (surprise! :P)
//
// In matrix notation:
// |X2|    |cos(Angle)   sin(Angle)| |X1|
// |  | =  |                       | |  |
// |Y2|    |-sin(Angle   cos(Angle)| |Y1|

static void DSP1_rotate(INT16 *input, INT16 *output)
{
   INT16 Angle = input[0];
   INT16 X1 = input[1];
   INT16 Y1 = input[2];
   INT16* X2 = &output[0];
   INT16* Y2 = &output[1];

   *X2 = (Y1 * DSP1_sin(Angle) >> 15) + (X1 * DSP1_cos(Angle) >> 15);
   *Y2 = (Y1 * DSP1_cos(Angle) >> 15) - (X1 * DSP1_sin(Angle) >> 15);
}

//////////////////////////////////////////////////////////////////

// Calculate the coordinates (X2, Y2, Z2) obtained when rotating (X1, Y1, Z1)
// three-dimensionally. Rotation is done in the order of Az around the Z axis,
// Ay around the Y axis and Ax around the X axis. As occur with the "attitude" commands
// (see comments in the "gyrate" command), qthis doesn't match what explained in
// the official documentation, but it's coherent with what it is done in the "attitude"
// command (but not with the "gyrate" command).
//
// In matrix notation:
// |X2|   |1      0      0  | |cosRy   0   -sinRy| | cosRz  sinRz    0| |X1|
// |Y2| = |0    cosRx  sinRx| |  0     1      0  | |-sinRz  cosRz    0| |Y1|
// |Z2|   |0   -sinRx  cosRx| |sinRy   0    cosRy| |   0      0      1| |Z1|

static void DSP1_polar(INT16 *input, INT16 *output)
{
   INT16 Az = input[0];
   INT16 Ay = input[1];
   INT16 Ax = input[2];
   INT16 X1 = input[3];
   INT16 Y1 = input[4];
   INT16 Z1 = input[5];
   INT16* X2 = &output[0];
   INT16* Y2 = &output[1];
   INT16* Z2 = &output[2];

   INT16 X, Y, Z;

   // Rotate Around Z
   X = (Y1 * DSP1_sin(Az) >> 15) + (X1 * DSP1_cos(Az) >> 15);
   Y = (Y1 * DSP1_cos(Az) >> 15) - (X1 * DSP1_sin(Az) >> 15);
   X1 = X; Y1 = Y;

   // Rotate Around Y
   Z = (X1 * DSP1_sin(Ay) >> 15) + (Z1 * DSP1_cos(Ay) >> 15);
   X = (X1 * DSP1_cos(Ay) >> 15) - (Z1 * DSP1_sin(Ay) >> 15);
   *X2 = X; Z1 = Z;

   // Rotate Around X
   Y = (Z1 * DSP1_sin(Ax) >> 15) + (Y1 * DSP1_cos(Ax) >> 15);
   Z = (Z1 * DSP1_cos(Ax) >> 15) - (Y1 * DSP1_sin(Ax) >> 15);
   *Y2 = Y; *Z2 = Z;
}

//////////////////////////////////////////////////////////////////

// Set up the elements of an "attitude matrix" (there are other ones):
//           S | cosRz  sinRz    0| |cosRy   0   -sinRy| |1      0      0  |
// MatrixA = - |-sinRz  cosRz    0| |  0     1      0  | |0    cosRx  sinRx|
//           2 |   0      0      1| |sinRy   0    cosRy| |0   -sinRx  cosRx|
// This matrix is thought to be used within the following framework:
// let's suppose we define positive rotations around a system of orthogonal axes in this manner:
// a rotation of +90 degrees around axis3 converts axis2 into axis1
// a rotation of +90 degrees around axis2 converts axis1 into axis3
// a rotation of +90 degrees around axis1 converts axis3 into axis2
// and let's suppose that we have defined a new orthonormal axes system (FLU)
// by doing the following operations about the standard one (XYZ):
// first rotating the XYZ system around Z by an angle Rz (obtaining X'Y'Z'),
// then rotating the resulting system around Y by an angle Ry (obtaining X''Y''Z'')
// and, finally, rotating the resulting system around X by an angle Rx (obtaining FLU)
// This FLU (forward/left/up) system represents an "attitude" and, then, the matrix here defined
// is the change of coordinates matrix that transform coordinates in the FLU
// system (the "object coordinates") into the standard XYZ system (the "global coordinates"),
// multiplied by a scale factor S/2, that is:
// |x|   S             |f|
// |y| * - = MatrixA * |l|
// |z|   2             |u|
// In a similar way, if we use the transpose of the matrix, we can transform global coordinates
// into object coordinates:
// |f|   S                        |x|
// |l| * - = MatrixA_transposed * |y|
// |u|   2                        |z|
//
// input[0]: S
// input[1]: Rz
// input[2]: Ry
// input[3]: Rx

static void DSP1_attitudeA(INT16 *input, INT16 *output)
{
   INT16 S = input[0];
   INT16 Rz = input[1];
   INT16 Ry = input[2];
   INT16 Rx = input[3];

   INT16 SinRz = DSP1_sin(Rz);
   INT16 CosRz = DSP1_cos(Rz);
   INT16 SinRy = DSP1_sin(Ry);
   INT16 CosRy = DSP1_cos(Ry);
   INT16 SinRx = DSP1_sin(Rx);
   INT16 CosRx = DSP1_cos(Rx);

   S >>= 1;

   shared.MatrixA[0][0] = (S * CosRz >> 15) * CosRy >> 15;
   shared.MatrixA[0][1] = ((S * SinRz >> 15) * CosRx >> 15) + (((S * CosRz >> 15) * SinRx >> 15) * SinRy >> 15);
   shared.MatrixA[0][2] = ((S * SinRz >> 15) * SinRx >> 15) - (((S * CosRz >> 15) * CosRx >> 15) * SinRy >> 15);

   shared.MatrixA[1][0] = -((S * SinRz >> 15) * CosRy >> 15);
   shared.MatrixA[1][1] = ((S * CosRz >> 15) * CosRx >> 15) - (((S * SinRz >> 15) * SinRx >> 15) * SinRy >> 15);
   shared.MatrixA[1][2] = ((S * CosRz >> 15) * SinRx >> 15) + (((S * SinRz >> 15) * CosRx >> 15) * SinRy >> 15);

   shared.MatrixA[2][0] = S * SinRy >> 15;
   shared.MatrixA[2][1] = -((S * SinRx >> 15) * CosRy >> 15);
   shared.MatrixA[2][2] = (S * CosRx >> 15) * CosRy >> 15;
}

//////////////////////////////////////////////////////////////////

// Same than 'attitudeA', but with a difference attitude matrix (matrixB)

static void DSP1_attitudeB(INT16 *input, INT16 *output)
{
   INT16 S = input[0];
   INT16 Rz = input[1];
   INT16 Ry = input[2];
   INT16 Rx = input[3];

   INT16 SinRz = DSP1_sin(Rz);
   INT16 CosRz = DSP1_cos(Rz);
   INT16 SinRy = DSP1_sin(Ry);
   INT16 CosRy = DSP1_cos(Ry);
   INT16 SinRx = DSP1_sin(Rx);
   INT16 CosRx = DSP1_cos(Rx);

   S >>= 1;

   shared.MatrixB[0][0] = (S * CosRz >> 15) * CosRy >> 15;
   shared.MatrixB[0][1] = ((S * SinRz >> 15) * CosRx >> 15) + (((S * CosRz >> 15) * SinRx >> 15) * SinRy >> 15);
   shared.MatrixB[0][2] = ((S * SinRz >> 15) * SinRx >> 15) - (((S * CosRz >> 15) * CosRx >> 15) * SinRy >> 15);

   shared.MatrixB[1][0] = -((S * SinRz >> 15) * CosRy >> 15);
   shared.MatrixB[1][1] = ((S * CosRz >> 15) * CosRx >> 15) - (((S * SinRz >> 15) * SinRx >> 15) * SinRy >> 15);
   shared.MatrixB[1][2] = ((S * CosRz >> 15) * SinRx >> 15) + (((S * SinRz >> 15) * CosRx >> 15) * SinRy >> 15);

   shared.MatrixB[2][0] = S * SinRy >> 15;
   shared.MatrixB[2][1] = -((S * SinRx >> 15) * CosRy >> 15);
   shared.MatrixB[2][2] = (S * CosRx >> 15) * CosRy >> 15;
}

//////////////////////////////////////////////////////////////////

// Same than 'attitudeA', but with a difference attitude matrix (matrixC)

static void DSP1_attitudeC(INT16 *input, INT16 *output)
{
   INT16 S = input[0];
   INT16 Rz = input[1];
   INT16 Ry = input[2];
   INT16 Rx = input[3];

   INT16 SinRz = DSP1_sin(Rz);
   INT16 CosRz = DSP1_cos(Rz);
   INT16 SinRy = DSP1_sin(Ry);
   INT16 CosRy = DSP1_cos(Ry);
   INT16 SinRx = DSP1_sin(Rx);
   INT16 CosRx = DSP1_cos(Rx);

   S >>= 1;

   shared.MatrixC[0][0] = (S * CosRz >> 15) * CosRy >> 15;
   shared.MatrixC[0][1] = ((S * SinRz >> 15) * CosRx >> 15) + (((S * CosRz >> 15) * SinRx >> 15) * SinRy >> 15);
   shared.MatrixC[0][2] = ((S * SinRz >> 15) * SinRx >> 15) - (((S * CosRz >> 15) * CosRx >> 15) * SinRy >> 15);

   shared.MatrixC[1][0] = -((S * SinRz >> 15) * CosRy >> 15);
   shared.MatrixC[1][1] = ((S * CosRz >> 15) * CosRx >> 15) - (((S * SinRz >> 15) * SinRx >> 15) * SinRy >> 15);
   shared.MatrixC[1][2] = ((S * CosRz >> 15) * SinRx >> 15) + (((S * SinRz >> 15) * CosRx >> 15) * SinRy >> 15);

   shared.MatrixC[2][0] = S * SinRy >> 15;
   shared.MatrixC[2][1] = -((S * SinRx >> 15) * CosRy >> 15);
   shared.MatrixC[2][2] = (S * CosRx >> 15) * CosRy >> 15;
}

//////////////////////////////////////////////////////////////////

// Convert global coordinates (X,Y,Z) to object coordinates (F,L,U)
// See the comment in "attitudeA" for a explanation about the calculation.
//
// input[0]: X ; input[1]: Y ; input[2]: Z
// &output[0]: F ; &output[1]: L ; &output[2]: U

static void DSP1_objectiveA(INT16 *input, INT16 *output)
{
   INT16 X = input[0];
   INT16 Y = input[1];
   INT16 Z = input[2];
   INT16* F = &output[0];
   INT16* L = &output[1];
   INT16* U = &output[2];

   *F = (shared.MatrixA[0][0] * X >> 15) + (shared.MatrixA[1][0] * Y >> 15) + (shared.MatrixA[2][0] * Z >> 15);
   *L = (shared.MatrixA[0][1] * X >> 15) + (shared.MatrixA[1][1] * Y >> 15) + (shared.MatrixA[2][1] * Z >> 15);
   *U = (shared.MatrixA[0][2] * X >> 15) + (shared.MatrixA[1][2] * Y >> 15) + (shared.MatrixA[2][2] * Z >> 15);
}

//////////////////////////////////////////////////////////////////

// Same than 'objectiveA', but for the 'B' attitude

static void DSP1_objectiveB(INT16 *input, INT16 *output)
{
   INT16 X = input[0];
   INT16 Y = input[1];
   INT16 Z = input[2];
   INT16* F = &output[0];
   INT16* L = &output[1];
   INT16* U = &output[2];

   *F = (shared.MatrixB[0][0] * X >> 15) + (shared.MatrixB[1][0] * Y >> 15) + (shared.MatrixB[2][0] * Z >> 15);
   *L = (shared.MatrixB[0][1] * X >> 15) + (shared.MatrixB[1][1] * Y >> 15) + (shared.MatrixB[2][1] * Z >> 15);
   *U = (shared.MatrixB[0][2] * X >> 15) + (shared.MatrixB[1][2] * Y >> 15) + (shared.MatrixB[2][2] * Z >> 15);
}

//////////////////////////////////////////////////////////////////

// Same than 'objectiveA', but for the 'C' attitude

static void DSP1_objectiveC(INT16 *input, INT16 *output)
{
   INT16 X = input[0];
   INT16 Y = input[1];
   INT16 Z = input[2];
   INT16* F = &output[0];
   INT16* L = &output[1];
   INT16* U = &output[2];

   *F = (shared.MatrixC[0][0] * X >> 15) + (shared.MatrixC[1][0] * Y >> 15) + (shared.MatrixC[2][0] * Z >> 15);
   *L = (shared.MatrixC[0][1] * X >> 15) + (shared.MatrixC[1][1] * Y >> 15) + (shared.MatrixC[2][1] * Z >> 15);
   *U = (shared.MatrixC[0][2] * X >> 15) + (shared.MatrixC[1][2] * Y >> 15) + (shared.MatrixC[2][2] * Z >> 15);
}

//////////////////////////////////////////////////////////////////

// Convert object coordinates (F,L,U) to object coordinates (X,Y,Z)
// See the comment in "attitudeA" for a explanation about the calculation.
//
// input[0]: F ; input[1]: L ; input[2]: U
// &output[0]: X ; &output[1]: Y ; &output[2]: Z

static void DSP1_subjectiveA(INT16 *input, INT16 *output)
{
   INT16 F = input[0];
   INT16 L = input[1];
   INT16 U = input[2];
   INT16* X = &output[0];
   INT16* Y = &output[1];
   INT16* Z = &output[2];

   *X = (shared.MatrixA[0][0] * F >> 15) + (shared.MatrixA[0][1] * L >> 15) + (shared.MatrixA[0][2] * U >> 15);
   *Y = (shared.MatrixA[1][0] * F >> 15) + (shared.MatrixA[1][1] * L >> 15) + (shared.MatrixA[1][2] * U >> 15);
   *Z = (shared.MatrixA[2][0] * F >> 15) + (shared.MatrixA[2][1] * L >> 15) + (shared.MatrixA[2][2] * U >> 15);
}

//////////////////////////////////////////////////////////////////

// Same than 'subjectiveA', but for the 'B' attitude

static void DSP1_subjectiveB(INT16 *input, INT16 *output)
{
   INT16 F = input[0];
   INT16 L = input[1];
   INT16 U = input[2];
   INT16* X = &output[0];
   INT16* Y = &output[1];
   INT16* Z = &output[2];

   *X = (shared.MatrixB[0][0] * F >> 15) + (shared.MatrixB[0][1] * L >> 15) + (shared.MatrixB[0][2] * U >> 15);
   *Y = (shared.MatrixB[1][0] * F >> 15) + (shared.MatrixB[1][1] * L >> 15) + (shared.MatrixB[1][2] * U >> 15);
   *Z = (shared.MatrixB[2][0] * F >> 15) + (shared.MatrixB[2][1] * L >> 15) + (shared.MatrixB[2][2] * U >> 15);
}

//////////////////////////////////////////////////////////////////

// Same than 'subjectiveA', but for the 'C' attitude

static void DSP1_subjectiveC(INT16 *input, INT16 *output)
{
   INT16 F = input[0];
   INT16 L = input[1];
   INT16 U = input[2];
   INT16* X = &output[0];
   INT16* Y = &output[1];
   INT16* Z = &output[2];

   *X = (shared.MatrixC[0][0] * F >> 15) + (shared.MatrixC[0][1] * L >> 15) + (shared.MatrixC[0][2] * U >> 15);
   *Y = (shared.MatrixC[1][0] * F >> 15) + (shared.MatrixC[1][1] * L >> 15) + (shared.MatrixC[1][2] * U >> 15);
   *Z = (shared.MatrixC[2][0] * F >> 15) + (shared.MatrixC[2][1] * L >> 15) + (shared.MatrixC[2][2] * U >> 15);
}

//////////////////////////////////////////////////////////////////

// This command calculates the inner product (S) of a vector (X,Y,Z) and
// the first column of MatrixA. It should be noted that that first column
// represent the global coordinates of an unity vector in the forward
// direction in the object coordinate system (coordinates (1,0,0) in the FLU
// axes system).
//
// input[0]: X ; input[1]: Y ; input[2]: Z
// &output[0]: S

static void DSP1_scalarA(INT16 *input, INT16 *output)
{
   INT16 X = input[0];
   INT16 Y = input[1];
   INT16 Z = input[2];
   INT16* S = &output[0];

   *S = (X * shared.MatrixA[0][0] + Y * shared.MatrixA[1][0] + Z * shared.MatrixA[2][0]) >> 15;
}

//////////////////////////////////////////////////////////////////

// Same than 'scalarA', but for the 'B' attitude

static void DSP1_scalarB(INT16 *input, INT16 *output)
{
   INT16 X = input[0];
   INT16 Y = input[1];
   INT16 Z = input[2];
   INT16* S = &output[0];

   *S = (X * shared.MatrixB[0][0] + Y * shared.MatrixB[1][0] + Z * shared.MatrixB[2][0]) >> 15;
}

//////////////////////////////////////////////////////////////////

// Same than 'scalarA', but for the 'C' attitude

static void DSP1_scalarC(INT16 *input, INT16 *output)
{
   INT16 X = input[0];
   INT16 Y = input[1];
   INT16 Z = input[2];
   INT16* S = &output[0];

   *S = (X * shared.MatrixC[0][0] + Y * shared.MatrixC[1][0] + Z * shared.MatrixC[2][0]) >> 15;
}

//////////////////////////////////////////////////////////////////

// This command determines the final attitude angles after the body with attitude angles (Ax, Ay, Az) with
// respect to the global coordinates is rotated by the minor angular displacements (DeltaF, DeltaL, DeltaU).
// It means that the XYZ axes are rotated by (Ax, Ay, Az) to obtain the FLU axes and, then, these
// are rotated by (DeltaF, DeltaL, DeltaU). The command calculates and return the new FLU angles respect to the
// XYZ system (Rx, Ry, Rz)
// The formulae are:
// Rx = Ax + (DeltaU*sin(Ay)+DeltaF*cos(Ay))
// Ry = Ay + DeltaL - tan(Ax)*(DeltaU*cos(Ay)+DeltaF*sin(Ay))
// Rz = Az + sec(Ax)*(DeltaU*cos(Ay)-DeltaF*sin(Ay))
//
// Now the discussion: according to the official documentation, as described in various commands, you pass from
// XYZ to FLU by doing the rotations in the order Y, X, Z. In this command, the formulae are coherent with the
// fact that Y is the first axis to do a rotation around it. However, in the "attitude" command, while the official
// document describe it that way, we have discovered, when reverse engineering the command, that the calculated
// matrix do the rotation around Y in the second place. This incoherent behaviour of various commands is, in my
// opinion, a pretty severe implementation error. However, if you only use small "minor displacements", the error term
// introduced by that incoherence should be almost negligible.

static void DSP1_gyrate(INT16 *input, INT16 *output)
{
   INT16 Az = input[0];
   INT16 Ax = input[1];
   INT16 Ay = input[2];
   INT16 U = input[3];
   INT16 F = input[4];
   INT16 L = input[5];
   INT16* Rz = &output[0];
   INT16* Rx = &output[1];
   INT16* Ry = &output[2];

   INT16 CSec, ESec, CSin, C, E;
   INT16 SinAy = DSP1_sin(Ay);
   INT16 CosAy = DSP1_cos(Ay);

   inverse(DSP1_cos(Ax), 0, &CSec, &ESec);

   // Rotation Around Z
   normalizeDouble(U * CosAy - F * SinAy, &C, &E);

   E = ESec - E;

   normalize(C * CSec >> 15, &C, &E);

   *Rz = Az + denormalizeAndClip(C, E);

   // Rotation Around X
   *Rx = Ax + (U * SinAy >> 15) + (F * CosAy >> 15);

   // Rotation Around Y
   normalizeDouble(U * CosAy + F * SinAy, &C, &E);

   E = ESec - E;

   normalize(DSP1_sin(Ax), &CSin, &E);

   normalize(-(C * (CSec * CSin >> 15) >> 15), &C, &E);

   *Ry = Ay + denormalizeAndClip(C, E) + L;
}

//////////////////////////////////////////////////////////////////

static const INT16 DSP1_MaxAZS_Exp[16] = {
   0x38b4, 0x38b7, 0x38ba, 0x38be, 0x38c0, 0x38c4, 0x38c7, 0x38ca,
   0x38ce, 0x38d0, 0x38d4, 0x38d7, 0x38da, 0x38dd, 0x38e0, 0x38e4
};

//////////////////////////////////////////////////////////////////


// Set-up the projection framework. Besides returning some values, it store in RAM some values that
// will be used by the other three projection commands (raster, target an project)
// Input:
// (Fx, Fy, Fz)-> coordinates of base point (global coordinates)
// Lfe-> distance between the base point and the viewpoint (center of projection)
// Les-> distance between the base point and the screen
// Aas-> azimuth angle (0 degrees is east; 90 degrees is north)
// Azs-> zenith angle (0 degrees is zenith)
// Output:
// Vof-> raster line of imaginary center (whatever it means ;) )
// Vva-> raster line representing the horizon line
// (Cx, Cy)-> coordinates of the projection of the center of the screen over the ground (ground coordinates)

static void DSP1_parameter(INT16 *input, INT16 *output)
{
   INT16 Fx = input[0];
   INT16 Fy = input[1];
   INT16 Fz = input[2];
   INT16 Lfe = input[3];
   INT16 Les = input[4];
   INT16 Aas = input[5];
   INT16 Azs = input[6];
   INT16* Vof = &output[0];
   INT16* Vva = &output[1];
   INT16* Cx = &output[2];
   INT16* Cy = &output[3];

   INT16 CSec, C, E;
   INT16 LfeNx, LfeNy, LfeNz;
   INT16 LesNx, LesNy, LesNz;
   INT16 AZS, MaxAZS;

   // Copy Zenith angle for clipping
   AZS = Azs;

   // Store Les and his coefficient and exponent when normalized
   shared.Les = Les;
   shared.E_Les=0;
   normalize(Les, &shared.C_Les, &shared.E_Les);

   // Store Sine and Cosine of Azimuth and Zenith angle
   shared.SinAas = DSP1_sin(Aas);
   shared.CosAas = DSP1_cos(Aas);
   shared.SinAzs = DSP1_sin(Azs);
   shared.CosAzs = DSP1_cos(Azs);

   // normal vector to the screen (norm 1, points toward the center of projection)
   shared.Nx = shared.SinAzs * -shared.SinAas >> 15;
   shared.Ny = shared.SinAzs * shared.CosAas >> 15;
   shared.Nz = shared.CosAzs * 0x7fff >> 15;

   // horizontal vector of the screen (Hz=0, norm 1, points toward the right of the screen)
   shared.Hx = shared.CosAas*0x7fff>>15;
   shared.Hy = shared.SinAas*0x7fff>>15;

   // vertical vector of the screen (norm 1, points toward the top of the screen)
   shared.Vx = shared.CosAzs*-shared.SinAas>>15;
   shared.Vy = shared.CosAzs*shared.CosAas>>15;
   shared.Vz = -shared.SinAzs*0x7fff>>15;

   LfeNx = Lfe*shared.Nx>>15;
   LfeNy = Lfe*shared.Ny>>15;
   LfeNz = Lfe*shared.Nz>>15;

   // Center of Projection
   shared.CentreX = Fx+LfeNx;
   shared.CentreY = Fy+LfeNy;
   shared.CentreZ = Fz+LfeNz;

   LesNx = Les*shared.Nx>>15;
   LesNy = Les*shared.Ny>>15;
   LesNz = Les*shared.Nz>>15;

   // center of the screen (global coordinates)
   shared.Gx=shared.CentreX-LesNx;
   shared.Gy=shared.CentreY-LesNy;
   shared.Gz=shared.CentreZ-LesNz;


   E = 0;
   normalize(shared.CentreZ, &C, &E);

   shared.CentreZ_C = C;
   shared.CentreZ_E = E;

   // Determine clip boundary and clip Zenith angle if necessary
   // (Why to clip? Maybe to avoid the screen can only show sky with no ground? Only a guess...)
   MaxAZS = DSP1_MaxAZS_Exp[-E];

   if (AZS < 0) {
      MaxAZS = -MaxAZS;
      if (AZS < MaxAZS + 1) AZS = MaxAZS + 1;
   } else {
      if (AZS > MaxAZS) AZS = MaxAZS;
   }

   // Store Sine and Cosine of clipped Zenith angle
   shared.SinAZS = DSP1_sin(AZS);
   shared.CosAZS = DSP1_cos(AZS);

   // calculate the separation of (cx, cy) from the projection of
   // the 'centre of projection' over the ground... (CentreZ*tg(AZS))
   inverse(shared.CosAZS, 0, &shared.SecAZS_C1, &shared.SecAZS_E1);
   normalize(C * shared.SecAZS_C1 >> 15, &C, &E);
   E += shared.SecAZS_E1;
   C = denormalizeAndClip(C, E) * shared.SinAZS >> 15;

   // ... and then take into account the position of the centre of
   // projection and the azimuth angle
   shared.CentreX += C * shared.SinAas >> 15;
   shared.CentreY -= C * shared.CosAas >> 15;

   *Cx = shared.CentreX;
   *Cy = shared.CentreY;

   // Raster number of imaginary center and horizontal line
   *Vof = 0;

   if ((Azs != AZS) || (Azs == MaxAZS))
   {
      INT16 Aux;

      // correct vof and vva when Azs is outside the 'non-clipping interval'
      // we have only some few Taylor coefficients, so we cannot guess which ones
      // are the approximated functions and, what is worse, we don't know why
      // the own clipping stuff (and, particularly, this correction) is done
      if (Azs == -32768) Azs = -32767;

      C = Azs - MaxAZS;
      if (C >= 0) C--;
      Aux = ~(C << 2);

      // Vof += x+(1/3)*x^3, where x ranges from 0 to PI/4 when Azs-MaxAZS goes from 0 to 0x2000
      C = Aux * DSP1_DataRom[0x0328] >> 15;
      C = (C * Aux >> 15) + DSP1_DataRom[0x0327];
      *Vof -= (C * Aux >> 15) * Les >> 15;

      // CosAZS *= 1+(1/2)*x^2+(5/24)*x^24, where x ranges from 0 to PI/4 when Azs-MaxAZS goes from 0 to 0x2000
      C = Aux * Aux >> 15;
      Aux = (C * DSP1_DataRom[0x0324] >> 15) + DSP1_DataRom[0x0325];
      shared.CosAZS += (C * Aux >> 15) * shared.CosAZS >> 15;
   }

   // vertical offset of the screen with regard to the horizontal plane
   // containing the centre of projection
   shared.VOffset = Les * shared.CosAZS >> 15;

   // The horizon line (the line in the screen that is crossed by the horizon plane
   // -the horizontal plane containing the 'centre of projection'-),
   // will be at distance Les*cotg(AZS) from the centre of the screen. This is difficult
   // to explain but easily seen in a graph. To better see it, consider it in this way:
   // Les*tg(AZS-90), draw some lines and apply basic trigonometry. ;)
   inverse(shared.SinAZS, 0, &CSec, &E);
   normalize(shared.VOffset, &C, &E);
   normalize(C * CSec >> 15, &C, &E);

   if (C == -32768) { C >>= 1; E++; }

   *Vva = denormalizeAndClip(-C, E);

   // Store Secant of clipped Zenith angle
   inverse(shared.CosAZS, 0, &shared.SecAZS_C2, &shared.SecAZS_E2);
}

//////////////////////////////////////////////////////////////////

// Calculates the matrix which transform an object situated on a raster line (Vs) into
// his projection over the ground. The modified SecAZS is used here, so
// i don't understand the fine details, but, basically, it's done
// this way: The vertical offset between the point of projection and the
// raster line is calculated (Vs*SinAzs>>15)+VOffset, then the height of
// the center of projection is measured in that units (*CentreZ_C). If, now
// you consider the "reference case" (center of projection at an unit of height),
// the projection of a thin strip containing the raster line will have the same
// width (as the raster line would be on the ground in this case, but will suffer a
// change of scale in height (as the ground and the vertical axis would form an angle of 180-Azs degrees).
// This scale factor, when the angle 'center of screen-center of projection-raster line' is small,
// can be aproximated by the one of the center of the screen, 1/cos(Azs).(**) (Here is when it's used
// SecAZS). By last, you have to consider the effect of the azimuth angle Aas, and you are done.
//
// Using matrix notation:
//                    |A     B|    Centre_ZS     | cos(Aas)   -sin(Aas)|   |1           0|
// ProjectionMatrix = |       | = ----------- *  |                     | * |             |
//                    |C     D|   Vs*sin(Azs)    |sin(Aas)     cos(Aas)|   |0    sec(Azs)|
//
// (**)
// If Les=1, the vertical offset between the center
// of projection and the center of the screen is Cos(Azs); then, if the vertical
// offset is 1, the ratio of the projection over the ground respect to the
// line on the screen is 1/cos(Azs).

static void DSP1_raster(INT16 *input, INT16 *output)
{
   INT16 Vs = input[0];
   INT16* An = &output[0];
   INT16* Bn = &output[1];
   INT16* Cn = &output[2];
   INT16* Dn = &output[3];

   INT16 C, E, C1, E1;

   inverse((Vs * shared.SinAzs >> 15) + shared.VOffset, 7, &C, &E);

   E += shared.CentreZ_E;
   C1 = C * shared.CentreZ_C >> 15;

   E1 = E + shared.SecAZS_E2;

   normalize(C1, &C, &E);
   C = denormalizeAndClip(C, E);

   *An = C * shared.CosAas >> 15;
   *Cn = C * shared.SinAas >> 15;

   normalize(C1 * shared.SecAZS_C2 >> 15, &C, &E1);
   C = denormalizeAndClip(C, E1);

   *Bn = C * -shared.SinAas >> 15;
   *Dn = C * shared.CosAas >> 15;
}

//////////////////////////////////////////////////////////////////

// Calculate the projection over the ground of a selected point of screen
// It simply apply the projection matrix described in the "Raster" command
// to the vector (H,V) transposed, and add the result to the position of
// the centre of projection.
// The only special point to take into account is the directions on the screen:
// H is positive rightward, but V is positive downward; this is why
// the signs take that configuration

static void DSP1_target(INT16 *input, INT16 *output)
{
   INT16 H = input[0];
   INT16 V = input[1];
   INT16* X = &output[0];
   INT16* Y = &output[1];

   INT16 C, E, C1, E1;

   inverse((V * shared.SinAzs >> 15) + shared.VOffset, 8, &C, &E);

   E += shared.CentreZ_E;
   C1 = C * shared.CentreZ_C >> 15;

   E1 = E + shared.SecAZS_E1;

   H <<= 8;
   normalize(C1, &C, &E);
   C = denormalizeAndClip(C, E) * H >> 15;

   *X = shared.CentreX + (C * shared.CosAas >> 15);
   *Y = shared.CentreY - (C * shared.SinAas >> 15);

   V <<= 8;
   normalize(C1 * shared.SecAZS_C1 >> 15, &C, &E1);
   C = denormalizeAndClip(C, E1) * V >> 15;

   *X += C * -shared.SinAas >> 15;
   *Y += C * shared.CosAas >> 15;
}

//////////////////////////////////////////////////////////////////

// Calculation of the projection over the screen (H,V) of an object (X,Y,Z) and his
// 'enlargement ratio' (M). The positive directions on the screen are as described
// in the targe command. M is scaled down by 2^-7, that is, M==0x0100 means ratio 1:1

static void DSP1_project(INT16 *input, INT16 *output)
{
   INT16 X = input[0];
   INT16 Y = input[1];
   INT16 Z = input[2];
   INT16* H = &output[0];
   INT16* V = &output[1];
   INT16* M = &output[2];

   INT32 aux, aux4;
   INT16 E, E2, E3, E4, E5, refE, E6, E7;
   INT16 C2, C4, C6, C8, C9, C10, C11, C12, C16, C17, C18, C19, C20, C21, C22, C23, C24, C25, C26;
   INT16 Px, Py, Pz;

   E4=E3=E2=E=E5=0;

   normalizeDouble((INT32)(X)-shared.Gx, &Px, &E4);
   normalizeDouble((INT32)(Y)-shared.Gy, &Py, &E);
   normalizeDouble((INT32)(Z)-shared.Gz, &Pz, &E3);
   Px>>=1; E4--;   // to avoid overflows when calculating the scalar products
   Py>>=1; E--;
   Pz>>=1; E3--;

   refE = (E<E3)?E:E3;
   refE = (refE<E4)?refE:E4;

   Px=shiftR(Px,E4-refE);    // normalize them to the same exponent
   Py=shiftR(Py,E-refE);
   Pz=shiftR(Pz,E3-refE);

   C11=- (Px*shared.Nx>>15);
   C8=- (Py*shared.Ny>>15);
   C9=- (Pz*shared.Nz>>15);
   C12=C11+C8+C9;   // this cannot overflow!

   aux4=C12;   // de-normalization with 32-bits arithmetic
   refE = 16-refE;    // refE can be up to 3
   if (refE>=0)
      aux4 <<=(refE);
   else
      aux4 >>=-(refE);
   if (aux4==-1) aux4 = 0;      // why?
   aux4>>=1;

   aux = (UINT16)(shared.Les) + aux4;   // Les - the scalar product of P with the normal vector of the screen
   normalizeDouble(aux, &C10, &E2);
   E2 = 15-E2;

   inverse(C10, 0, &C4, &E4);
   C2=C4*shared.C_Les>>15;                 // scale factor


   // H
   E7=0;
   C16= (Px*shared.Hx>>15);
   C20= (Py*shared.Hy>>15);
   C17=C16+C20;   // scalar product of P with the normalized horizontal vector of the screen...

   C18=C17*C2>>15;    // ... multiplied by the scale factor
   normalize(C18, &C19, &E7);
   *H=denormalizeAndClip(C19, shared.E_Les-E2+refE+E7);

   // V
   E6=0;
   C21 = Px*shared.Vx>>15;
   C22 = Py*shared.Vy>>15;
   C23 = Pz*shared.Vz>>15;
   C24=C21+C22+C23;   // scalar product of P with the normalized vertical vector of the screen...

   C26=C24*C2>>15;    // ... multiplied by the scale factor
   normalize(C26, &C25, &E6);
   *V=denormalizeAndClip(C25, shared.E_Les-E2+refE+E6);

   // M
   normalize(C2, &C6, &E4);
   *M=denormalizeAndClip(C6, E4+shared.E_Les-E2-7); // M is the scale factor divided by 2^7
}

//////////////////////////////////////////////////////////////////

// Calculate the sine of the input parameter
// this is done by linear interpolation between
// the points of a look-up table

static INT16 DSP1_sin(INT16 Angle)
{
   INT32 S;

   if (Angle < 0) {
      if (Angle == -32768) return 0;
      return -sin(-Angle);
   }
   S = DSP1_SinTable[Angle >> 8] + (DSP1_MulTable[Angle & 0xff] * DSP1_SinTable[0x40 + (Angle >> 8)] >> 15);
   if (S > 32767) S = 32767;
   return (INT16) S;
}

//////////////////////////////////////////////////////////////////

// Calculate the cosine of the input parameter.
// It's used the same method than in sin(INT16)

static INT16 DSP1_cos(INT16 Angle)
{
   INT32 S;

   if (Angle < 0) {
      if (Angle == -32768) return -32768;
      Angle = -Angle;
   }
   S = DSP1_SinTable[0x40 + (Angle >> 8)] - (DSP1_MulTable[Angle & 0xff] * DSP1_SinTable[Angle >> 8] >> 15);
   if (S < -32768) S = -32767;
   return (INT16) S;
}

//////////////////////////////////////////////////////////////////

// Determines the inverse of a floating point decimal number
// iCoefficient*2^iExponent = 1/(Coefficient*2^Exponent), with the output
// normalized (iCoefficient represents a number whose absolute value is between 1/2 and 1)
// To invert 'Coefficient' a first initial guess is taken from a look-up table
// and, then, two iterations of the Newton method (applied to the function
// f(x)=1/(2*x)-Coefficient) are done. This results in a close approximation (iCoefficient) to a number 'y'
// that verify Coefficient*y=1/2. This is why you have to correct the exponent by one
// unit at the end.

static void inverse(INT16 Coefficient, INT16 Exponent, INT16 *iCoefficient, INT16 *iExponent)
{
   // Step One: Division by Zero
   if (Coefficient == 0x0000)
   {
      *iCoefficient = 0x7fff;
      *iExponent = 0x002f;
   }
   else
   {
      INT16 Sign = 1;

      // Step Two: Remove Sign
      if (Coefficient < 0)
      {
         if (Coefficient < -32767) Coefficient = -32767;
         Coefficient = -Coefficient;
         Sign = -1;
      }

      // Step Three: Normalize
      while (Coefficient < 0x4000)
      {
         Coefficient <<= 1;
         Exponent--;
      }

      // Step Four: Special Case
      if (Coefficient == 0x4000)
         if (Sign == 1) *iCoefficient = 0x7fff;
      else  {
         *iCoefficient = -0x4000;
         Exponent--;
      }
      else {
         // Step Five: Initial Guess
         INT16 i = DSP1_DataRom[((Coefficient - 0x4000) >> 7) + 0x0065];

         // Step Six: Iterate Newton's Method
         i = (i + (-i * (Coefficient * i >> 15) >> 15)) << 1;
         i = (i + (-i * (Coefficient * i >> 15) >> 15)) << 1;

         *iCoefficient = i * Sign;
      }

      *iExponent = 1 - Exponent;
   }
}

//////////////////////////////////////////////////////////////////

static INT16 denormalizeAndClip(INT16 C, INT16 E)
{
   if (E > 0) {
      if (C > 0) return 32767; else if (C < 0) return -32767;
   } else {
      if (E < 0) return C * DSP1_DataRom[0x0031 + E] >> 15;
   }
   return C;
}

//////////////////////////////////////////////////////////////////

// Normalize the input number (m), understood as ranging from -1 to 1,
// to the form: Coefficient*2^Exponent,
// where the absolute value of Coefficient is >= 1/2
// (Coefficient>=0x4000 or Coefficient <= (INT16)0xc001)

static void normalize(INT16 m, INT16 *Coefficient, INT16 *Exponent)
{
   INT16 i = 0x4000;
   INT16 e = 0;

   if (m < 0)
      while ((m & i) && i)
   {
      i >>= 1;
      e++;
   }
   else
      while (!(m & i) && i)
   {
      i >>= 1;
      e++;
   }

   if (e > 0)
      *Coefficient = m * DSP1_DataRom[0x21 + e] << 1;
   else
      *Coefficient = m;

   *Exponent -= e;
}

//////////////////////////////////////////////////////////////////

// Same than 'normalize' but with an INT32 input

static void normalizeDouble(INT32 Product, INT16 *Coefficient, INT16 *Exponent)
{
   INT16 n = Product & 0x7fff;
   INT16 m = Product >> 15;
   INT16 i = 0x4000;
   INT16 e = 0;

   if (m < 0)
      while ((m & i) && i)
   {
      i >>= 1;
      e++;
   }
   else
      while (!(m & i) && i)
   {
      i >>= 1;
      e++;
   }

   if (e > 0)
   {
      *Coefficient = m * DSP1_DataRom[0x0021 + e] << 1;

      if (e < 15)
         *Coefficient += n * DSP1_DataRom[0x0040 - e] >> 15;
      else
      {
         i = 0x4000;

         if (m < 0)
            while ((n & i) && i)
         {
            i >>= 1;
            e++;
         }
         else
            while (!(n & i) && i)
         {
            i >>= 1;
            e++;
         }

         if (e > 15)
            *Coefficient = n * DSP1_DataRom[0x0012 + e] << 1;
         else
            *Coefficient += n;
      }
   }
   else
      *Coefficient = m;

   *Exponent = e;
}

//////////////////////////////////////////////////////////////////

// Shift to the right

static INT16 shiftR(INT16 C, INT16 E)
{
   return (C * DSP1_DataRom[0x0031 + E] >> 15);
}

//////////////////////////////////////////////////////////////////

static const INT16 DSP1_SinTable[256] = {
   0x0000,  0x0324,  0x0647,  0x096a,  0x0c8b,  0x0fab,  0x12c8,  0x15e2,
   0x18f8,  0x1c0b,  0x1f19,  0x2223,  0x2528,  0x2826,  0x2b1f,  0x2e11,
   0x30fb,  0x33de,  0x36ba,  0x398c,  0x3c56,  0x3f17,  0x41ce,  0x447a,
   0x471c,  0x49b4,  0x4c3f,  0x4ebf,  0x5133,  0x539b,  0x55f5,  0x5842,
   0x5a82,  0x5cb4,  0x5ed7,  0x60ec,  0x62f2,  0x64e8,  0x66cf,  0x68a6,
   0x6a6d,  0x6c24,  0x6dca,  0x6f5f,  0x70e2,  0x7255,  0x73b5,  0x7504,
   0x7641,  0x776c,  0x7884,  0x798a,  0x7a7d,  0x7b5d,  0x7c29,  0x7ce3,
   0x7d8a,  0x7e1d,  0x7e9d,  0x7f09,  0x7f62,  0x7fa7,  0x7fd8,  0x7ff6,
   0x7fff,  0x7ff6,  0x7fd8,  0x7fa7,  0x7f62,  0x7f09,  0x7e9d,  0x7e1d,
   0x7d8a,  0x7ce3,  0x7c29,  0x7b5d,  0x7a7d,  0x798a,  0x7884,  0x776c,
   0x7641,  0x7504,  0x73b5,  0x7255,  0x70e2,  0x6f5f,  0x6dca,  0x6c24,
   0x6a6d,  0x68a6,  0x66cf,  0x64e8,  0x62f2,  0x60ec,  0x5ed7,  0x5cb4,
   0x5a82,  0x5842,  0x55f5,  0x539b,  0x5133,  0x4ebf,  0x4c3f,  0x49b4,
   0x471c,  0x447a,  0x41ce,  0x3f17,  0x3c56,  0x398c,  0x36ba,  0x33de,
   0x30fb,  0x2e11,  0x2b1f,  0x2826,  0x2528,  0x2223,  0x1f19,  0x1c0b,
   0x18f8,  0x15e2,  0x12c8,  0x0fab,  0x0c8b,  0x096a,  0x0647,  0x0324,
   -0x0000, -0x0324, -0x0647, -0x096a, -0x0c8b, -0x0fab, -0x12c8, -0x15e2,
   -0x18f8, -0x1c0b, -0x1f19, -0x2223, -0x2528, -0x2826, -0x2b1f, -0x2e11,
   -0x30fb, -0x33de, -0x36ba, -0x398c, -0x3c56, -0x3f17, -0x41ce, -0x447a,
   -0x471c, -0x49b4, -0x4c3f, -0x4ebf, -0x5133, -0x539b, -0x55f5, -0x5842,
   -0x5a82, -0x5cb4, -0x5ed7, -0x60ec, -0x62f2, -0x64e8, -0x66cf, -0x68a6,
   -0x6a6d, -0x6c24, -0x6dca, -0x6f5f, -0x70e2, -0x7255, -0x73b5, -0x7504,
   -0x7641, -0x776c, -0x7884, -0x798a, -0x7a7d, -0x7b5d, -0x7c29, -0x7ce3,
   -0x7d8a, -0x7e1d, -0x7e9d, -0x7f09, -0x7f62, -0x7fa7, -0x7fd8, -0x7ff6,
   -0x7fff, -0x7ff6, -0x7fd8, -0x7fa7, -0x7f62, -0x7f09, -0x7e9d, -0x7e1d,
   -0x7d8a, -0x7ce3, -0x7c29, -0x7b5d, -0x7a7d, -0x798a, -0x7884, -0x776c,
   -0x7641, -0x7504, -0x73b5, -0x7255, -0x70e2, -0x6f5f, -0x6dca, -0x6c24,
   -0x6a6d, -0x68a6, -0x66cf, -0x64e8, -0x62f2, -0x60ec, -0x5ed7, -0x5cb4,
   -0x5a82, -0x5842, -0x55f5, -0x539b, -0x5133, -0x4ebf, -0x4c3f, -0x49b4,
   -0x471c, -0x447a, -0x41ce, -0x3f17, -0x3c56, -0x398c, -0x36ba, -0x33de,
   -0x30fb, -0x2e11, -0x2b1f, -0x2826, -0x2528, -0x2223, -0x1f19, -0x1c0b,
   -0x18f8, -0x15e2, -0x12c8, -0x0fab, -0x0c8b, -0x096a, -0x0647, -0x0324};

   //////////////////////////////////////////////////////////////////

// Optimised for Performance
static const INT16 DSP1_MulTable[256] = {
      0x0000,  0x0003,  0x0006,  0x0009,  0x000c,  0x000f,  0x0012,  0x0015,
      0x0019,  0x001c,  0x001f,  0x0022,  0x0025,  0x0028,  0x002b,  0x002f,
      0x0032,  0x0035,  0x0038,  0x003b,  0x003e,  0x0041,  0x0045,  0x0048,
      0x004b,  0x004e,  0x0051,  0x0054,  0x0057,  0x005b,  0x005e,  0x0061,
      0x0064,  0x0067,  0x006a,  0x006d,  0x0071,  0x0074,  0x0077,  0x007a,
      0x007d,  0x0080,  0x0083,  0x0087,  0x008a,  0x008d,  0x0090,  0x0093,
      0x0096,  0x0099,  0x009d,  0x00a0,  0x00a3,  0x00a6,  0x00a9,  0x00ac,
      0x00af,  0x00b3,  0x00b6,  0x00b9,  0x00bc,  0x00bf,  0x00c2,  0x00c5,
      0x00c9,  0x00cc,  0x00cf,  0x00d2,  0x00d5,  0x00d8,  0x00db,  0x00df,
      0x00e2,  0x00e5,  0x00e8,  0x00eb,  0x00ee,  0x00f1,  0x00f5,  0x00f8,
      0x00fb,  0x00fe,  0x0101,  0x0104,  0x0107,  0x010b,  0x010e,  0x0111,
      0x0114,  0x0117,  0x011a,  0x011d,  0x0121,  0x0124,  0x0127,  0x012a,
      0x012d,  0x0130,  0x0133,  0x0137,  0x013a,  0x013d,  0x0140,  0x0143,
      0x0146,  0x0149,  0x014d,  0x0150,  0x0153,  0x0156,  0x0159,  0x015c,
      0x015f,  0x0163,  0x0166,  0x0169,  0x016c,  0x016f,  0x0172,  0x0175,
      0x0178,  0x017c,  0x017f,  0x0182,  0x0185,  0x0188,  0x018b,  0x018e,
      0x0192,  0x0195,  0x0198,  0x019b,  0x019e,  0x01a1,  0x01a4,  0x01a8,
      0x01ab,  0x01ae,  0x01b1,  0x01b4,  0x01b7,  0x01ba,  0x01be,  0x01c1,
      0x01c4,  0x01c7,  0x01ca,  0x01cd,  0x01d0,  0x01d4,  0x01d7,  0x01da,
      0x01dd,  0x01e0,  0x01e3,  0x01e6,  0x01ea,  0x01ed,  0x01f0,  0x01f3,
      0x01f6,  0x01f9,  0x01fc,  0x0200,  0x0203,  0x0206,  0x0209,  0x020c,
      0x020f,  0x0212,  0x0216,  0x0219,  0x021c,  0x021f,  0x0222,  0x0225,
      0x0228,  0x022c,  0x022f,  0x0232,  0x0235,  0x0238,  0x023b,  0x023e,
      0x0242,  0x0245,  0x0248,  0x024b,  0x024e,  0x0251,  0x0254,  0x0258,
      0x025b,  0x025e,  0x0261,  0x0264,  0x0267,  0x026a,  0x026e,  0x0271,
      0x0274,  0x0277,  0x027a,  0x027d,  0x0280,  0x0284,  0x0287,  0x028a,
      0x028d,  0x0290,  0x0293,  0x0296,  0x029a,  0x029d,  0x02a0,  0x02a3,
      0x02a6,  0x02a9,  0x02ac,  0x02b0,  0x02b3,  0x02b6,  0x02b9,  0x02bc,
      0x02bf,  0x02c2,  0x02c6,  0x02c9,  0x02cc,  0x02cf,  0x02d2,  0x02d5,
      0x02d8,  0x02db,  0x02df,  0x02e2,  0x02e5,  0x02e8,  0x02eb,  0x02ee,
      0x02f1,  0x02f5,  0x02f8,  0x02fb,  0x02fe,  0x0301,  0x0304,  0x0307,
      0x030b,  0x030e,  0x0311,  0x0314,  0x0317,  0x031a,  0x031d,  0x0321};

//////////////////////////////////////////////////////////////////

// Data ROM, as logged from a DSP-1B with the 0x1f command;
// it contains the tables and constants used by the commands.
// The tables used are: two shift tables (0x022-0x031 and 0x031-0x040 -this last one
// with an error in 0x03c which has survived to all the DSP-1 revisions-); a inverse
// table (used as initial guess) at 0x065-0x0e4; a square root table (used also
// as initial guess) at 0x0e5-0x115; two sin and cos tables (used as nodes to construct
// a interpolation curve) at, respectively, 0x116-0x197 and 0x196-0x215.
// As a curiosity, in the positions 0x21c-0x31c it's contained a
// 257-points arccos table that, apparently, have been not used anywhere
// (maybe for the MaxAZS_Exp table?).
static UINT16 DSP1_DataRom[1024];
