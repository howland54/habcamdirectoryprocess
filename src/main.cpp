#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QtAlgorithms>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QProcess>

#include <exiv2/exiv2.hpp>


#include <opencv2/imgcodecs.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/imgproc.hpp>

#include "tiffio.h"

#define  MAX_IMAGE_ROW 4080

QString  globalSourceDirectory;
QString  globalDestinationDirectory;
int      globalSourceDirectoryLength;
bool     leftJpegs;
bool     rightJpegs;
QString  globalNewDirName;
int imagesProcesedCount;
int globalSkipAmount;

void ProcessDir(QString srcPath, QString destPath,int skipThisMany)
{
   QDir dir(srcPath);                            //Opens the path
   QFileInfoList files = dir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot|QDir::Files, QDir::DirsFirst); //Gets the file information
   foreach(const QFileInfo &fi, files)
      {      //Loops through the found files.
         QString Path = fi.absoluteFilePath();  //Gets the absolute file path
         QDir destinationCopyDir;
         //QFileInfo outDirInfo;
         //QString newDirName;
         if(fi.isDir())
            {
               QString copyPath = Path;
               if(Path.startsWith("@eaD"))
                   {
                       continue;
                   }

               if(Path.endsWith(".tif")) // check to make sure nobody has run thumbs plus on this
                   {
                       continue;
                   }
               copyPath.remove(0,globalSourceDirectoryLength);
               globalNewDirName = destPath + "/" + copyPath;
               destinationCopyDir.mkpath(globalNewDirName);
               ProcessDir(Path,destPath,skipThisMany);          //Recursively goes through all the directories.
            }
         else
            {

               if(Path.endsWith(".tif"))
                  {

                     //char  newFileName[2048];
                     int doIProcess = imagesProcesedCount % globalSkipAmount;
                     if(!doIProcess)
                        {
                           QFileInfo   fileInfo(Path);
                           // 201509.20151014.154313644
                           QString theFilename =fileInfo.fileName();

                           QString theOutDirName = globalNewDirName;
                           char  eData[4096];
                           TIFF *inImage = TIFFOpen(Path.toLatin1().data(),"r");
                           char *localTOC;

                           TIFFGetField(inImage, TIFFTAG_IMAGEDESCRIPTION,&localTOC);
                           int stlen = strlen(localTOC);
                           if(stlen > 4095)
                              {
                                 stlen = 4095;
                              }
                           int i = 0;
                           for( i=0;i<stlen;i++) eData[i] = localTOC[i];
                           eData[i] = '\0';
                           TIFFClose(inImage);



                           cv::Mat  image=cv::imread(Path.toLatin1().data(),cv::IMREAD_UNCHANGED);
                           cv::Mat  leftColorImage;
                           cv::Mat  rightColorImage;

                           cv::Mat  leftImage;
                           cv::Mat  rightImage;

                           cv::Mat leftJpegImage;
                           cv::Mat rightJpegImage;

                           leftImage.create(image.rows, image.cols/2,CV_16UC1);
                           rightImage.create(image.rows, image.cols/2,CV_16UC1);

                           for(int j = 0; j < image.rows; j++)
                              {
                                 for(int k = 0; k < image.cols/2; k++)
                                    {
                                       leftImage.at<unsigned short>(j,k)  = image.at<unsigned short>(j,k);
                                       rightImage.at<unsigned short>(j,k)  = image.at<unsigned short>(j,k+image.cols/2);
                                    }
                              }

                           leftColorImage.create(image.rows, image.cols/2,CV_16UC3);
                           rightColorImage.create(image.rows, image.cols/2,CV_16UC3);

                           if(leftJpegs)
                              {
                                 cv::cvtColor(leftImage,leftColorImage, cv::COLOR_BayerBG2BGR,0);
                                 cv::Mat eightBitLeft = leftColorImage.clone();
                                 leftJpegImage = cv::Mat(image.rows, image.cols/2, CV_8UC3);
                                 //eightBitLeft.convertTo(eightBitLeft, CV_8UC3, 0.0625);
                                 leftColorImage.convertTo(leftJpegImage, CV_8UC3, 0.00390625);
                                 QString theNewFileName = theOutDirName + "/L" + theFilename;
                                 theNewFileName.replace(".tif",".jpg");

                                 cv::imwrite(theNewFileName.toLatin1().data(),leftJpegImage);

                                 Exiv2::Image::AutoPtr jpegImage = Exiv2::ImageFactory::open(theNewFileName.toUtf8().data());
                                 jpegImage->readMetadata();
                                 Exiv2::ExifData &exifData = jpegImage->exifData();
                                 exifData["Exif.Image.ImageDescription"] = eData;
                                 jpegImage->writeMetadata();
                              }
                           if(rightJpegs)
                              {
                                 cv::cvtColor(rightImage,rightColorImage, cv::COLOR_BayerBG2BGR,0);
                                 cv::Mat eightBitRight= rightColorImage.clone();
                                 rightJpegImage = cv::Mat(image.rows, image.cols/2, CV_8UC3);
                                 //eightBitRight.convertTo(eightBitRight, CV_8UC3, 0.0625);
                                 rightColorImage.convertTo(rightJpegImage, CV_8UC3, 0.00390625);
                                 QString theNewFileName = theOutDirName + "/R" + theFilename;
                                 theNewFileName.replace(".tif",".jpg");

                                 cv::imwrite(theNewFileName.toLatin1().data(),rightJpegImage);

                                 Exiv2::Image::AutoPtr jpegImage = Exiv2::ImageFactory::open(theNewFileName.toUtf8().data());
                                 jpegImage->readMetadata();
                                 Exiv2::ExifData &exifData = jpegImage->exifData();
                                 exifData["Exif.Image.ImageDescription"] = eData;
                                 jpegImage->writeMetadata();
                              }



                           // now convert to 8 bits






                          /*

                           QString program = "/usr/bin/jhead";
                           QStringList arguments;
                           QString eDataString = (QString)eData;

                           arguments << "-cl" << "\"" + eDataString + "\"" << theNewFileName;

                           QProcess *myProcess = new QProcess();
                           myProcess->execute(program, arguments);*/
                        }
                     imagesProcesedCount += 1;



                  }
               //qDebug()  << Path;

            }
      }
}


int main(int argc, char *argv[])
{
   QCoreApplication a(argc, argv);
   QCoreApplication::setApplicationName("habcamDirectoryProcess");
   QCoreApplication::setApplicationVersion("2.0");

   QCommandLineParser parser;
   parser.setApplicationDescription("Processes habcam image directory tree, version 2");
   parser.addHelpOption();
   parser.addVersionOption();




   QCommandLineOption skipOption(QStringList() << "s" << "skip", QCoreApplication::translate("main", "skip images"),QCoreApplication::translate("main", "images skipped"),"100");
   parser.addOption(skipOption);

   QCommandLineOption leftJpegOption(QStringList() << "l" << "left", QCoreApplication::translate("main", "produce left jpegs"));
   parser.addOption(leftJpegOption);


   QCommandLineOption rightJpegOption(QStringList() << "r" << "right", QCoreApplication::translate("main", "produce right jpegs"));
   parser.addOption(rightJpegOption);



   QCommandLineOption inputDirectoryOption(QStringList() << "i" << "input", QCoreApplication::translate("main", "input directory"),QCoreApplication::translate("main", "input"),"noDirectory");
   parser.addOption(inputDirectoryOption);

   QCommandLineOption outputDirectoryOption(QStringList() << "o" << "output", QCoreApplication::translate("main", "output directory"),QCoreApplication::translate("main", "output"),"noDirectory");
   parser.addOption(outputDirectoryOption);

   // Process the actual command line arguments given by the user
   parser.process(a);


   bool skipImages = parser.isSet(skipOption);
   int  skipAmount = 1;
   if(skipImages)
      {
         skipAmount = parser.value(skipOption).toInt();
         if((skipAmount > 1000) || (skipAmount <= 0))
            {
               skipAmount = 100;
            }
      }
   globalSkipAmount = skipAmount;

   leftJpegs= parser.isSet(leftJpegOption);
   rightJpegs = parser.isSet(rightJpegOption);

   if((!leftJpegs ) && (!rightJpegs))
      {
         leftJpegs = true;
      }

   QString inputDirectory = parser.value(inputDirectoryOption);
   QString outputDirectory = parser.value(outputDirectoryOption);



   if(inputDirectory == "noDirectory")
      {
         fputs(qPrintable(parser.helpText()), stderr);
         return 0;
      }

   if(outputDirectory == "noDirectory")
      {
         fputs(qPrintable(parser.helpText()), stderr);
         return 0;
      }


   QDir  srcDir(inputDirectory);
   QDir  destDir(outputDirectory);

   globalSourceDirectory = inputDirectory;
   globalSourceDirectoryLength = globalSourceDirectory.length();
   if(!globalSourceDirectory.endsWith("/"))
      {
         globalSourceDirectoryLength +=1;
      }

   globalDestinationDirectory = outputDirectory;
   imagesProcesedCount = 0;

   Exiv2::XmpParser::initialize();
   ::atexit(Exiv2::XmpParser::terminate);


   if(!destDir.exists())
      {
         destDir.mkdir(outputDirectory);
      }
   if(srcDir.exists())
      {
         ProcessDir(inputDirectory,outputDirectory,  skipAmount);
      }

   return 0;
}


