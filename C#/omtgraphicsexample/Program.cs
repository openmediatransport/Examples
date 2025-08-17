using libomtnet;
using SkiaSharp;
using System.Runtime.InteropServices;

/*
 * OMT Graphics Example
 * 
 * This example generates a horizontal ticker graphic and ends it via an OMT Sender.
 * 
 * This demonstrates the alpha channel functionality built into Open Media Transport.
 * 
 */

namespace omtgraphicsexample
{
    internal class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("OMT Graphics Example");

            //Create the graphics format we will be sending, in this example this is a standard 1080p video.
            SKImageInfo imageInfo = new SKImageInfo(1920, 1080, SKColorType.Bgra8888, SKAlphaType.Unpremul);

            //This is the surface we will be drawing to to create out frame
            SKSurface image = SKSurface.Create(imageInfo);

            //Canvas contains all the drawing functions we will be utilising
            SKCanvas canvas = image.Canvas;

            //Define a rectangle for our ticker
            SKRect rect = new SKRect(0, 960, 1920, 1080);
            SKRoundRect roundRect = new SKRoundRect(rect, 8);

            //Create a simple gradient background for our ticker
            SKShader gradient = SKShader.CreateLinearGradient(new SKPoint(rect.Left, rect.Top), new SKPoint(rect.Right, rect.Bottom), new[] {
                    new SKColor(0, 19, 69, 192),   // semi-transparent deep blue
                    new SKColor(0, 89, 254, 192)  // semi-transparent light blue
                }, null, SKShaderTileMode.Clamp);
            SKPaint gradientPaint = new SKPaint() { Shader = gradient };

            //Create the text formatting to use
            SKFont textFont = new SKFont(SKTypeface.FromFamilyName("Sans Serif", SKFontStyle.Bold), 48);
            SKPaint textPaint = new SKPaint() { IsAntialias = true, Color = SKColors.White };

            //Position text in the middle of the gradient rectangle
            SKFontMetrics fontMetrics = new SKFontMetrics();
            textFont.GetFontMetrics(out fontMetrics);
            SKPoint textPosition =new SKPoint(rect.Right, rect.Top + ((rect.Height - (fontMetrics.Ascent + fontMetrics.Descent)) / 2f));

            //This is the text we will be sending along with an estimate of its width to know when to repeat.
            string tickerText = "This is an example of text rendering in SkiaSharp that is sent over Open Media Transport!";
            float tickerTextWidth = textFont.MeasureText(tickerText);

            //Create our sender
            using (OMTSend send = new OMTSend("Graphics", OMTQuality.Default))
            {

                //Create the media frame that contains the final graphic ready for sending
                OMTMediaFrame frame = new OMTMediaFrame();
                frame.Type = OMTFrameType.Video;
                frame.Codec = (int)OMTCodec.BGRA;
                frame.Width = imageInfo.Width;
                frame.Height = imageInfo.Height;
                frame.FrameRate = 59.94F;
                frame.Flags = OMTVideoFlags.Alpha; //Set premultiplied here also if necessary
                frame.AspectRatio = (float)imageInfo.Width / (float)imageInfo.Height;
                frame.ColorSpace = OMTColorSpace.BT709;
                frame.Stride = imageInfo.Width * 4;
                frame.DataLength = frame.Stride * frame.Height;
                frame.Data = Marshal.AllocHGlobal(frame.DataLength);
                frame.Timestamp = -1; //Important, informs OMT to automatically pace our frames to the frame rate.

                Console.WriteLine("Sending graphics on: \"" + send.Address + "\"");
                Console.WriteLine("");

                while (true)
                {
                    canvas.Clear(); //Clear our canvas so it is fully transparent
                    canvas.DrawRoundRect(roundRect, gradientPaint);
                    canvas.DrawText(tickerText, textPosition, textFont, textPaint);                       

                    if (image.ReadPixels(imageInfo, frame.Data, frame.Stride, 0, 0))
                    {
                        send.Send(frame);
                        Console.Write(".");
                    } else
                    {
                        Console.WriteLine("Failed to read pixels from SkiaSharp Surface, exiting....");
                        break;
                    }

                    //Update text position to scroll along the screen
                    textPosition.X -= 4;
                    if (textPosition.X < -tickerTextWidth) { textPosition.X = rect.Width; }

                }
            }

        }
    }
}
