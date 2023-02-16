#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <stdlib.h>
#include <QDebug>
#include <stdio.h>
#include <math.h>
MainWindow::MainWindow(QWidget* parent)
{
    parent = new QMainWindow;
    ui = new Ui::MainWindow;
    ui->setupUi(this);
    
    m_numBuffers = 10;

    m_frequency = 50.0;
    ui->frequencySlider->setRange(50,1000);
    ui->frequencySlider->setValue(m_frequency);
    ui->frequencySlider->setSliderPosition(m_frequency);
    ui->frequencySlider->setTickInterval(10);
    ui->frequencySlider->setTickPosition(QSlider::TicksAbove);
    ui->frequencyLabel->setText("Frequency:" + QString::number(m_frequency) + " Hz");

    m_numChannels = 2;
    ui->numChannels->setChecked(0);

    ui->sampleRate->addItem("8000");
    ui->sampleRate->addItem("16000");
    ui->sampleRate->addItem("44100");
    ui->sampleRate->addItem("48000");
    ui->sampleRate->addItem("96000");
    ui->sampleRate->setCurrentIndex(2);


    m_maxAmplitude = 32768.0;
    
    m_wavBitDepth = 16;
    m_blockAlign = m_numChannels * m_wavBitDepth/8;
    m_wavByteRate = m_sampleRate * m_blockAlign;

    m_numSamplesPerBuffer = WAV_BUFFERSIZE/m_blockAlign;
}

MainWindow::~MainWindow()
{
    if(m_file_wav.exists())
        m_file_wav.close();
    if(m_file_mp3.exists())
        m_file_mp3.close();
    lame_close(m_lgf);
    delete ui;
}

void MainWindow::on_numChannels_stateChanged(int state)
{
    if(state)
        m_numChannels = 1;
    else
        m_numChannels = 2;
    m_blockAlign = m_numChannels * m_wavBitDepth/8;
    m_wavByteRate = m_sampleRate * m_blockAlign;
    m_numSamplesPerBuffer = WAV_BUFFERSIZE/m_blockAlign;

}
void MainWindow::on_sampleRate_currentIndexChanged(int currentIndex)
{
    QString option = ui->sampleRate->currentText();
    m_sampleRate = option.toInt();
    m_blockAlign = m_numChannels * m_wavBitDepth/8;
    m_wavByteRate = m_sampleRate * m_blockAlign;
    m_numSamplesPerBuffer = WAV_BUFFERSIZE/m_blockAlign;
}

void MainWindow::on_frequencySlider_valueChanged(int frequency)
{
    m_frequency = (double)(frequency);
    ui->frequencyLabel->setText("Frequency:" + QString::number(m_frequency) + " Hz");
}
void MainWindow::on_durationSlider_valueChanged(int seconds)
{
    m_duration = seconds;
    m_numBuffers = (m_sampleRate * m_wavBitDepth * seconds)/(8 * WAV_BUFFERSIZE);
    ui->durationLabel->setText("Duration:" + QString::number(seconds) + " sec");
}

QString MainWindow::generateFilePath()
{
    QString fileName = "tone_" + 
    QString::number(m_frequency) + "-Hz_" + 
    QString::number(m_duration) + "-sec_" + 
    QString::number(m_sampleRate) + "-persec_" + 
    (m_numChannels==1?"MONO_":"STEREO_");
    QString filePath = "/Users/nav/Projects_CCPP/qt5/QToneGenerator/" + fileName;
    return filePath;
}

void MainWindow::on_generateToneButton_clicked()
{
    QString filePath = generateFilePath();
    m_file_wav.setFileName(filePath + ".wav");
    m_file_wav.open(QFile::WriteOnly);
    m_stream_wav.setDevice(&m_file_wav);
    m_stream_wav.setByteOrder(QDataStream::LittleEndian);
    preparewavHeader();
    m_stream_wav.writeRawData((char*)&m_wavHeader,44);

    prepareEncoder();
    m_file_mp3.setFileName(filePath + ".mp3");
    m_file_mp3.open(QFile::WriteOnly);
    m_stream_mp3.setDevice(&m_file_mp3);
    m_stream_mp3.setByteOrder(QDataStream::LittleEndian);

    qDebug() << "num_samples = " << m_numSamplesPerBuffer;
    qint16 buffer_wav_left[m_numSamplesPerBuffer];
    qint16 buffer_wav_right[m_numSamplesPerBuffer];
    for (int i = 0; i < m_numBuffers; i++)
    {
        memset(buffer_wav_left,0,m_numSamplesPerBuffer*m_wavBitDepth/8);
        memset(buffer_wav_right,0,m_numSamplesPerBuffer*m_wavBitDepth/8);
        memset(m_buffer_wav,0,WAV_BUFFERSIZE);
        memset(m_buffer_mp3,0,MP3_BUFFERSIZE);
        for (int j = 0; j < m_numSamplesPerBuffer; j++)
        {
            double t = (double)(i*m_numSamplesPerBuffer+j)/m_sampleRate;
            double v = sin(2*M_PI*m_frequency*t);
            qint16 sample = (qint16)(v*m_maxAmplitude);
            // printf("%d|",sample);
            m_buffer_wav[2*j] = sample;
            m_buffer_wav[2*j+1] = sample;
            buffer_wav_left[j] = sample;
            buffer_wav_right[j] = sample;
        }

        int bytes_encoded = lame_encode_buffer(
            m_lgf,
            buffer_wav_left,buffer_wav_right,//left,right
            m_numSamplesPerBuffer,//sizeof wav_buffer
            m_buffer_mp3,
            MP3_BUFFERSIZE
            );
        int bytes_written_wav = m_stream_wav.writeRawData((char*)m_buffer_wav,WAV_BUFFERSIZE*sizeof(qint16));
        int bytes_written_mp3 = m_stream_mp3.writeRawData((char*)m_buffer_mp3,bytes_encoded);
        qDebug("Buffer# %d | Encoded %d | MP3 write %d | WAV write %d",i,bytes_encoded,bytes_written_mp3,bytes_written_wav);
    }
    int bytes_flushed_mp3 = lame_encode_flush(m_lgf,m_buffer_mp3,MP3_BUFFERSIZE);
    int bytes_written_mp3 = m_stream_mp3.writeRawData((char*)m_buffer_mp3,bytes_flushed_mp3);
    
    m_file_wav.close();
    m_file_mp3.close();

}

void MainWindow::prepareEncoder()
{
    m_lgf = lame_init();
    if(m_numChannels==1)
        lame_set_mode(m_lgf,MONO);
    if(m_numChannels==2)
        lame_set_mode(m_lgf,STEREO);
    lame_set_in_samplerate(m_lgf, m_sampleRate);
    lame_set_out_samplerate(m_lgf, m_sampleRate);
    lame_set_brate(m_lgf, 128);
    // lame_set_VBR_mean_bitrate_kbps(m_lgf, 128);
    lame_set_num_channels(m_lgf, m_numChannels);
    lame_init_params(m_lgf);
    qDebug() << "Encoder Ready";
}

void MainWindow::preparewavHeader()
{
    m_wavHeader.ChunkID[0]      = 'R';
    m_wavHeader.ChunkID[1]      = 'I';
    m_wavHeader.ChunkID[2]      = 'F';
    m_wavHeader.ChunkID[3]      = 'F';
    
    m_wavHeader.ChunkSize       = m_blockAlign*WAV_BUFFERSIZE*m_numBuffers+36;
    
    m_wavHeader.Format[0]       = 'W';
    m_wavHeader.Format[1]       = 'A';
    m_wavHeader.Format[2]       = 'V';
    m_wavHeader.Format[3]       = 'E';
    
    m_wavHeader.Subchunk1ID[0]  = 'f';
    m_wavHeader.Subchunk1ID[1]  = 'm';
    m_wavHeader.Subchunk1ID[2]  = 't';
    m_wavHeader.Subchunk1ID[3]  = ' ';
    
    m_wavHeader.Subchunk1Size   = SUBCHUNK1SIZE;
    m_wavHeader.AudioFormat     = AUDIO_FORMAT;
    m_wavHeader.NumChannels     = m_numChannels;
    m_wavHeader.SampleRate      = m_sampleRate;
    m_wavHeader.ByteRate        = m_wavByteRate;
    m_wavHeader.BlockAlign      = m_blockAlign;
    m_wavHeader.BitsPerSample   = m_wavBitDepth;
    
    m_wavHeader.Subchunk2ID[0]  = 'd';
    m_wavHeader.Subchunk2ID[1]  = 'a';
    m_wavHeader.Subchunk2ID[2]  = 't';
    m_wavHeader.Subchunk2ID[3]  = 'a';
    m_wavHeader.Subchunk2Size   = m_blockAlign*WAV_BUFFERSIZE*m_numBuffers;
}