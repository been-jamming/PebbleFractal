# PebbleFractal

  This is an app that runs on pebble time and pebble time steel watches. It can render julia and mandelbrot sets of any kind. The controls and UI are a bit sloppy, but I'll continue working to fix it.

Controls:
When user selected julia set:
  First number (real value for c in z = z^2+c):
    up - hold to make the number increase
    down - hold to make the nubmer decrease
    select - move on to the next number
  Second number (imaginary value for c in z = z^2+c):
    up - hold to make the number increase
    down - hold to make the number decrease
    select - move on to fractal rendering
  Fractal rendering:
    See fractal rendering controls
 Fractal rendering controls:
  //sorry, these are terrible. If I had access to the gyroscope, they wouldn't be...
  state = zoomstate
  if state is zoomstate:
    up - press to zoom in a little//takes a second because it has to re-render. This applies to the other adjustments too.
    down - press to zoom out a little
    up - doubletap to increase the rendering quality//this makes the renders take longer too
    down - doubletap to decrease the rendering quality//this makes the renders take a shorter amount of time too
    select - state = upanddownstate
  if state is upanddownstate:
    up - press to go up a little
    down - press to go down a little
    up - press multiple times in succession to go up that many increments//i.e. you don't have to wait for it to finish rendering
    down - press multiple times in succession to go down that many increments
    select - state = leftandrightstate
  if state is leftandrightstate:
    up - press to go to the right a little
    down - press to go to the left a little
    up - press multiple times in succession to go to the right that many increments
    down - press multiple times in succession to go to the left that many increments
    select - state = leftandrightstate
