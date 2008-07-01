#include "mandelbrot.h"
#include<QtGui/QPainter>
#include<QtGui/QImage>
#include<QtGui/QMouseEvent>
#include<QtCore/QTime>

void MandelbrotWidget::resizeEvent(QResizeEvent *)
{
  if(size < width() * height())
  {
    std::cout << "reallocate buffer" << std::endl;
    size = width() * height();
    if(buffer) delete[]buffer;
    buffer = new unsigned char[4*size];
  }
}

template<typename T> struct iters_before_test { enum { ret = 8 }; };
template<> struct iters_before_test<double> { enum { ret = 16 }; };

template<typename Real> void MandelbrotThread::render(int img_width, int img_height)
{
  enum { packetSize = Eigen::ei_packet_traits<Real>::size }; // number of reals in a Packet
  typedef Eigen::Matrix<Real, packetSize, 1> Packet; // wrap a Packet as a vector

  enum { iters_before_test = iters_before_test<Real>::ret };
  max_iter = (max_iter / iters_before_test) * iters_before_test;
  const int alignedWidth = (img_width/packetSize)*packetSize;
  unsigned char *const buffer = widget->buffer;
  const double xradius = widget->xradius;
  const double yradius = xradius * img_height / img_width;
  typedef Eigen::Matrix<Real, 2, 1> Vector2;
  Vector2 start(widget->center.x() - widget->xradius, widget->center.y() - yradius);
  Vector2 step(2*widget->xradius/img_width, 2*yradius/img_height);
  total_iter = 0;
  int slice_height = img_height / widget->threadcount;

  for(int y = id * slice_height; y < (id+1) * slice_height; y++)
  {
    int pix = y * img_width;

    // for each pixel, we're going to do the iteration z := z^2 + c where z and c are complex numbers, 
    // starting with z = c = complex coord of the pixel. pzi and pzr denote the real and imaginary parts of z.
    // pci and pcr denote the real and imaginary parts of c.

    Packet pzi_start, pci_start;
    for(int i = 0; i < packetSize; i++) pzi_start[i] = pci_start[i] = start.y() + y * step.y();

    for(int x = 0; x < alignedWidth; x += packetSize, pix += packetSize)
    {
      Packet pcr, pci = pci_start, pzr, pzi = pzi_start, pzr_buf;
      for(int i = 0; i < packetSize; i++) pzr[i] = pcr[i] = start.x() + (x+i) * step.x();

      // do the iterations. Every 4 iterations we check for divergence, in which case we can stop iterating.
      int j = 0;
      typedef Eigen::Matrix<int, packetSize, 1> Packeti;
      Packeti pix_iter = Packeti::zero(), pix_dont_diverge;
      do
      {
        for(int i = 0; i < iters_before_test/4; i++) // peel the inner loop by 4
        {
#         define ITERATE \
            pzr_buf = pzr; \
            pzr = pzr.cwiseAbs2() - pzi.cwiseAbs2() + pcr; \
            pzi = 2 * pzr_buf.cwiseProduct(pzi) + pci;
          ITERATE ITERATE ITERATE ITERATE
        }
        pix_dont_diverge = (pzr.cwiseAbs2() + pzi.cwiseAbs2())
                           .eval() // temporary fix as what follows is not yet vectorized by Eigen
                           .cwiseLessThan(Packet::constant(iters_before_test))
                           .template cast<int>();
        pix_iter += iters_before_test * pix_dont_diverge;
        j++;
        total_iter += iters_before_test * packetSize;
      }
      while(j < max_iter/iters_before_test && pix_dont_diverge.any()); // any() is not yet vectorized by Eigen

      // compute pixel colors
      for(int i = 0; i < packetSize; i++)
      {
        
        buffer[4*(pix+i)] = 255*pix_iter[i]/max_iter;
        buffer[4*(pix+i)+1] = 0;
        buffer[4*(pix+i)+2] = 0;
      }
    }

    // if the width is not a multiple of packetSize, fill the remainder in black
    for(int x = alignedWidth; x < img_width; x++, pix++)
      buffer[4*pix] = buffer[4*pix+1] = buffer[4*pix+2] = 0;
  }
  return;
}

void MandelbrotThread::run()
{
  setTerminationEnabled(true);
  double resolution = widget->xradius*2/widget->width();
  max_iter = 128;
  if(resolution < 1e-4f) max_iter += 128 * ( - 4 - std::log10(resolution));
  int img_width = widget->width()/widget->draft;
  int img_height = widget->height()/widget->draft;
  single_precision = resolution > 1e-7f;

  if(single_precision)
    render<float>(img_width, img_height);
  else
    render<double>(img_width, img_height);
}

void MandelbrotWidget::paintEvent(QPaintEvent *)
{
  static float max_speed = 0;
  long long total_iter = 0;

  QTime time;
  time.start();
  for(int th = 0; th < threadcount; th++)
    threads[th]->start(QThread::LowPriority);
  for(int th = 0; th < threadcount; th++)
  {
    threads[th]->wait();
    total_iter += threads[th]->total_iter;
  }
  int elapsed = time.elapsed();

  if(draft == 1)
  {
    float speed = elapsed ? float(total_iter)*1000/elapsed : 0;
    max_speed = std::max(max_speed, speed);
    std::cout << threadcount << " threads, "
              << elapsed << " ms, "
              << speed << " iters/s (max " << max_speed << ")" << std::endl;
    int packetSize = threads[0]->single_precision
                   ? int(Eigen::ei_packet_traits<float>::size)
                   : int(Eigen::ei_packet_traits<double>::size);
    setWindowTitle(QString("resolution ")+QString::number(xradius*2/width(), 'e', 2)
                  +QString(", up to %1 iterations").arg(threads[0]->max_iter)
                  +(threads[0]->single_precision ? QString(", single ") : QString(", double "))
                  +QString("precision, ")
                  +(packetSize==1 ? QString("no vectorization")
                                  : QString("vectorized (%1 per packet)").arg(packetSize)));
  }
  
  QImage image(buffer, width()/draft, height()/draft, QImage::Format_RGB32);
  QPainter painter(this);
  painter.drawImage(QPoint(0, 0), image.scaled(width(), height()));

  if(draft>1)
  {
    draft /= 2;
    setWindowTitle(QString("recomputing at 1/%1 resolution...").arg(draft));
    update();
  }
}

void MandelbrotWidget::mousePressEvent(QMouseEvent *event)
{
  if( event->buttons() & Qt::LeftButton )
  {
    lastpos = event->pos();
    double yradius = xradius * height() / width();
    center = Eigen::Vector2d(center.x() + (event->pos().x() - width()/2) * xradius * 2 / width(),
                             center.y() + (event->pos().y() - height()/2) * yradius * 2 / height());
    draft = 16;
    for(int th = 0; th < threadcount; th++)
      threads[th]->terminate();
    update();
  }
}

void MandelbrotWidget::mouseMoveEvent(QMouseEvent *event)
{
  QPoint delta = event->pos() - lastpos;
  lastpos = event->pos();
  if( event->buttons() & Qt::LeftButton )
  {
    double t = 1 + 5 * double(delta.y()) / height();
    if(t < 0.5) t = 0.5;
    if(t > 2) t = 2;
    xradius *= t;
    draft = 16;
    for(int th = 0; th < threadcount; th++)
      threads[th]->terminate();
    update();
  }
}

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  MandelbrotWidget w;
  w.show();
  return app.exec();
}

#include "mandelbrot.moc"
