/*
  ==============================================================================

    Histogram.cpp
    Created: 20 Oct 2022 9:08:59am
    Author:  andreas

  ==============================================================================
*/

#include "Histogram.h"

Histogram ::Histogram(int dataLevels) : dataLevels(dataLevels), fixedLengthData(false)
{
  dataEdit.enter();
  data = new float[dataLength * dataLevels];
  dataReady.set(true);
  dataEdit.exit();
}

Histogram ::Histogram(float *inData, int dataLevels, int dataLength) : dataLevels(dataLevels), dataLength(dataLength), fixedLengthData(true)
{
  dataEdit.enter();
  data = new float[dataLength * dataLevels];
std:
  memcpy(data, inData, sizeof(float) * dataLength * dataLevels);
  dataReady.set(true);
  dataEdit.exit();
}

Histogram ::~Histogram()
{
}

void Histogram::update()
{
  repaint();
}

juce::Colour Histogram::levelToColour(float level)
{
  return juce::Colour::fromHSV((1.0 - level) / 9.0 + 0.66, 0.9, level, 1.0);
}

void Histogram::addDataLine(const float dataLineIN[], bool nomalized)
{
  float dataLine[dataLevels];
  juce::FloatVectorOperations::copy(dataLine, dataLineIN, dataLevels);
  if (!nomalized)
  {
    auto minMax = juce::FloatVectorOperations::findMinAndMax(dataLine, dataLevels);
    juce::FloatVectorOperations::add(dataLine, -minMax.getStart(), dataLevels);
    juce::FloatVectorOperations::multiply(dataLine, 1.0 / (minMax.getEnd() - minMax.getStart()), dataLevels);
  }
  histogramImage.moveImageSection(0, 0, levelWidth, 0, widthAvailable - levelWidth, heightAvailable);
  for (auto j = widthAvailable - levelWidth; j < widthAvailable; ++j)
  {
    for (auto k = 0; k < heightAvailable; ++k)
    {
      histogramImage.setPixelAt(j, k, levelToColour(dataLine[heightDataMap[k]]));
    }
  }
  dataEdit.enter();
  dataReady.set(false);
  for (auto i = dataLength - 1; i > 0; --i)
  {
    std::memcpy(&data[i * dataLevels], &data[(i - 1) * dataLevels], sizeof(float) * dataLevels);
  }
  std::memcpy(data, dataLine, sizeof(float) * dataLevels);
  dataReady.set(true);
  dataEdit.exit();
}

void Histogram ::paint(juce::Graphics &g)
{
  // (Our component is opaque, so we must completely fill the background with a solid colour)
  g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
  g.drawImage(histogramImage, 0, 0, widthAvailable, heightAvailable, 0, 0, widthAvailable, heightAvailable);
}

void Histogram::replaceData(const float *inData, size_t inDataLength, bool normalized)
{
  auto newData = new float[inDataLength * dataLevels];
std:
  memcpy(newData, inData, sizeof(float) * inDataLength * dataLevels);
  if (!normalized)
  {
    for (auto i = 0; i < inDataLength; i++)
    {
      auto minMax = juce::FloatVectorOperations::findMinAndMax(&newData[i*dataLevels], dataLevels);
      juce::FloatVectorOperations::add(&newData[i*dataLevels], -minMax.getStart(), dataLevels);
      juce::FloatVectorOperations::multiply(&newData[i*dataLevels], 1.0 / (minMax.getEnd() - minMax.getStart()), dataLevels);
    }
  }
  float *temp = data;
  dataEdit.enter();
  dataReady.set(false);
  data = newData;
  fixedLengthData = true;
  dataLength = inDataLength;
  dataReady.set(true);
  dataEdit.exit();
  delete temp;
  resized();
}

void Histogram::resized()
{
  if (dataReady.get())
  {
    widthAvailable = getWidth();
    heightAvailable = getHeight();
    histogramImage = juce::Image(juce::Image::RGB, widthAvailable, heightAvailable, true);
    delete heightDataMap;
    heightDataMap = new int[heightAvailable];
    int pixelPerLevel = heightAvailable / dataLevels;
    int pixelLeftOver = heightAvailable % dataLevels;
    int oldDataLength = dataLength;
    if (!fixedLengthData)
    {
      levelWidth = std::min(pixelPerLevel + 1, 20);
      dataLength = widthAvailable / levelWidth + 1;
    }
    else
    {
      levelWidth = widthAvailable / dataLength + 1;
    }
    int level = 0;
    int pixelsLeft = pixelPerLevel;
    for (auto i = heightAvailable - 1; i >= 0; i--)
    {
      if (pixelsLeft == 0)
      {
        if (pixelLeftOver > 0)
        {
          heightDataMap[i] = level;
          level++;
          pixelLeftOver--;
          pixelsLeft = pixelPerLevel;
        }
        else
        {
          level++;
          pixelsLeft = pixelPerLevel;
          heightDataMap[i] = level;
          pixelsLeft--;
        }
      }
      else
      {
        heightDataMap[i] = level;
        pixelsLeft--;
      }
    }
    if (oldDataLength < dataLength)
    {
      float *newData = new float[dataLevels * dataLength];
      std::memcpy(newData, data, sizeof(float) * oldDataLength * dataLevels);
      float *temp = data;
      data = newData;
      delete temp;
    }
    for (auto i = 0; i < dataLength; ++i)
    {
      for (auto j = 0; j < levelWidth; ++j)
      {
        for (auto k = 0; k < heightAvailable; ++k)
        {
          int yCord = (i * levelWidth + j);
          histogramImage.setPixelAt(yCord, k, levelToColour(data[i * dataLevels + heightDataMap[k]]));
        }
      }
    }
  }
}