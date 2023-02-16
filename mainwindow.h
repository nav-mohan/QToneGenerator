#if !defined(MAINWINDOW_H)
#define MAINWINDOW_H

#include <QObject>
#include <QWidget>
#include <QMainWindow>
#include <QFile>
#include <QDataStream>
#include <stdint.h>
#include <lame/lame.h>


#define WAV_BUFFERSIZE  (4096)
#define SUBCHUNK1SIZE   (16)
#define AUDIO_FORMAT    (1) /*For PCM*/

#define MP3_BUFFERSIZE   ((int) (1.25 * WAV_BUFFERSIZE) + 7200)

typedef struct wavHeader
{
    char    ChunkID[4];     /*  4   */
    qint32  ChunkSize;      /*  4   */
    char    Format[4];      /*  4   */
    
    char    Subchunk1ID[4]; /*  4   */
    qint32  Subchunk1Size;  /*  4   */
    qint16  AudioFormat;    /*  2   */
    qint16  NumChannels;    /*  2   */
    qint32  SampleRate;     /*  4   */
    qint32  ByteRate;       /*  4   */
    qint16  BlockAlign;     /*  2   */
    qint16  BitsPerSample;  /*  2   */
    
    char    Subchunk2ID[4];
    qint32  Subchunk2Size;
} wavHeader;

namespace Ui{class MainWindow;} // namespace Ui


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void on_frequencySlider_valueChanged(int value);
    void on_durationSlider_valueChanged(int value);
    void on_generateToneButton_clicked();
    void on_numChannels_stateChanged(int);
    void on_sampleRate_currentIndexChanged(int);

private:
    Ui::MainWindow* ui;
    double m_frequency;
    quint32 m_numBuffers;
    int m_duration;

    uint8_t m_numChannels;
    uint32_t m_sampleRate;
    double m_maxAmplitude;
    uint8_t m_wavBitDepth;
    uint32_t m_wavByteRate;
    uint8_t m_blockAlign;
    uint32_t m_numSamplesPerBuffer;

    // uint16_t m_mpegBitRate;

    QString generateFilePath();

    QFile m_file_wav;
    qint16 m_buffer_wav[WAV_BUFFERSIZE];
    QDataStream m_stream_wav;
    wavHeader m_wavHeader;
    void preparewavHeader();
    
    lame_global_flags* m_lgf;
    QFile m_file_mp3;
    unsigned char m_buffer_mp3[MP3_BUFFERSIZE];
    QDataStream m_stream_mp3;
    void prepareEncoder();

};
#endif // MAINWINDOW_H
