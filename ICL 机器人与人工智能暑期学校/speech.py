# import speech_recognition as sr
#
#
# harvard = sr.AudioFile('harvard.wav')
# r = sr.Recognizer()
# with harvard as source:
#     audio = r.record(source)
#     print(r.recognize_google(audio))


# def wav2txt(wavfilepath):
#     r = sr.Recognizer()
#     sudio = ""
#     with sr.AudioFile(wavfilepath) as src:
#         sudio = r.record(src)
#         print(r.recognize_sphinx(sudio))
#
#
#
# if __name__ == '__main__':
#     wav2txt("harvard.wav")

import speech_recognition as sr

r = sr.Recognizer()
mic = sr.Microphone()
with mic as source:
    r.adjust_for_ambient_noise(source)
    audio = r.listen(source)


print(r.recognize_google(audio))
