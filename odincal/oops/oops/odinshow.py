import sys
import string
import odin

from Tkinter import *
import Pmw


class HeaderInfo:
    def __init__(self, frame, label, row):
        self.label = label
        Label(frame,
              text=string.capitalize(label + ':'),
              justify='right').grid(row=row, column=0, sticky=E)
        self.value = Label(frame, font='fixed',
                           width=16, background='white')  # ,relief='sunken'
        self.value.grid(row=row, column=1, sticky=W)

    def setValue(self, value):
        self.value.configure(text=str(value))


def setInfo(info, field, value):
    for i in range(len(info)):
        if info[i].label == field:
            info[i].setValue(value)


class SpectrumViewer:

    def __init__(self, master):
        self.master = master
        # master.minsize(1280,600)
        # master.minsize(1000,500)
        # master.minsize(900,550)
        master.minsize(800, 500)

        if not Pmw.Blt.haveblt(master):     # is Blt installed?
            print("BLT is not installed!")

        f = Frame(master)
        f.pack(side='top', expand=1, fill='both')

        self.g = Pmw.Blt.Graph(f)
        self.g.pack(side='left', expand=1, fill='both', padx=20, pady=20)

        colors = ["blue", "green", "red", "black",
                  "yellow", "cyan", "magenta", "grey"]

        for band in range(8):
            curvename = "band" + str(band)
            x = ()
            y = ()
            self.g.line_create(curvename, xdata=x, ydata=y,
                               color=colors[band], symbol='', label="")

        self.g.configure(title='Spectrum viewer')
        self.g.grid_on()
        self.showCrosshairs()

        f = Frame(f)
        f.pack(side='right', fill='y')
        g = Frame(f)
        g.pack(side='top', anchor=N, padx=10)

        Label(g, text='Header info').grid(row=0, columnspan=2, pady=5)
        self.info = []
        row = 1
        for field in ('stw', 'orbit', 'mjd', 'frontend', 'backend', 'type',
                      'source', 'ra2000', 'dec2000', 'quality', 'skybeamhit',
                      'inttime', 'skyfreq', 'restfreq'):
            self.info.append(HeaderInfo(g, field, row))
            row = row + 1

        g = Frame(f)
        g.pack(side='top', anchor=S, pady=10)
        Label(g, text='Cursor info').grid(row=0, columnspan=2, pady=5)
        self.xpos = Label(g, text='', font='fixed', width=12,
                          relief='sunken', background='white')
        self.ypos = Label(g, text='', font='fixed', width=12,
                          relief='sunken', background='white')
        self.xpos.grid(row=1, column=0)
        self.ypos.grid(row=1, column=1)

        f = Frame(f, relief='raised')
        f.pack(side='bottom', fill='x', pady=5)
        Button(f, text='Quit',
               command=master.quit).pack(side='bottom', padx=10)

    def fillHeader(self, s):
        info = self.info
        setInfo(info, 'stw', "%08X" % (s.stw))
        setInfo(info, 'orbit', "%10.4f" % (s.orbit))
        setInfo(info, 'mjd', "%10.3f" % (s.mjd))
        setInfo(info, 'frontend', s.frontend)
        setInfo(info, 'backend', s.backend)
        setInfo(info, 'type', s.type)
        setInfo(info, 'source', s.source)
        setInfo(info, 'ra2000', "%10.4f" % (s.ra2000))
        setInfo(info, 'dec2000', "%10.4f" % (s.dec2000))
        setInfo(info, 'skybeamhit', "%08X" % (s.skybeamhit))
        setInfo(info, 'quality', "%08X" % (s.quality))
        setInfo(info, 'inttime', "%10.3f" % (s.inttime))
        setInfo(info, 'skyfreq', "%10.4f" % (s.skyfreq / 1.0e9))
        setInfo(info, 'restfreq', "%10.4f" % (s.restfreq / 1.0e9))

    def showSpectrum(self, s):
        r = s.Copy()
        n = r.channels
        if n > 0:
            self.fillHeader(r)
            bands = 1
            # r.FreqSort()
            if r.backend != "AOS":
                if (r.quality & 0x02000000) == 0:
                    if r.intmode & 0x0100:
                        mode = r.intmode & 0x00ff
                        mode = mode >> 1
                        while mode > 0:
                            if mode & 1:
                                bands = bands + 1
                            mode = mode >> 1
                        if r.intmode & 0x0200:
                            bands = bands / 2
                    else:
                        mode = r.intmode & 0x0007
                        if mode in range(1, 5):
                            bands = 2**(mode - 1)
                        if bands > 1 and r.intmode & 0x10:
                            bands = bands / 2
            x = r.Frequency()
            y = r.data
            for band in range(bands):
                i0 = band * n / bands
                i1 = i0 + n / bands
                curvename = "band" + str(band)
                self.g.element_configure(curvename,
                                         xdata=tuple(x[i0:i1]),
                                         ydata=tuple(y[i0:i1]))
            x = ()
            y = ()
            for band in range(bands, 8):
                curvename = "band" + str(band)
                self.g.element_configure(curvename,
                                         xdata=tuple(x),
                                         ydata=tuple(y))
            # self.g.postscript_configure(landscape=1,
            #                             decorations=0,
            #                             paperheight="11i",
            #                             paperwidth="8i",
            #                             height=400, width=640)
            # self.g.postscript_output(fileName="show.ps")

    def showCrosshairs(self):
        hide = not int(self.g.crosshairs_cget('hide'))
        self.g.crosshairs_configure(hide=hide, dashes="1")
        if(hide):
            self.g.unbind('<Motion>')
        else:
            self.g.bind('<Motion>', self.mouseMove)

    def mouseDown(self, event):
        global dragging, x0, y0
        dragging = 0
        if self.g.inside(event.x, event.y):
            self.showCrosshairs()
            dragging = 1
            (x0, y0) = self.g.invtransform(event.x, event.y)
            self.g.marker_create("line",
                                 name="marking rectangle",
                                 dashes=(2, 2))
            self.g.bind("<Motion>", self.mouseDrag)

    def mouseMove(self, ev):
        self.g.crosshairs_configure(position="@" + str(ev.x) + "," + str(ev.y))
        x, y = self.g.invtransform(ev.x, ev.y)
        self.xpos.configure(text="%9.4f" % (x))
        self.ypos.configure(text="%9.4f" % (y))


if __name__ == '__main__':
    if len(sys.argv) > 1:
        root = Tk(className="Show")
        sv = SpectrumViewer(root)
        s = odin.Spectrum()
        for file in sys.argv[1:]:
            s.Load(file)
            sv.showSpectrum(s)
            root.update()
        sv.master.mainloop()
