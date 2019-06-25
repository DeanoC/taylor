local image = require "image"
local test = image.load("test_imageload.png")
if test == nil then
    print("test_imageload.png unable to be loaded")
end
local w = test:width();
local h = test:height();
local d = test:depth();
local s = test:slices();

if w ~= 884 then
    print("test_imageload.png is " .. w .. " wide it should be 884")
end
if h ~= 406 then
    print("test_imageload.png is " .. h .. " high it should be 406")
end
if d ~= 1 then
    print("test_imageload.png is " .. d .. " depth it should be 1")
end
if s ~= 1 then
    print("test_imageload.png is " .. s .. " slices it should be 1")
end

--image.saveAsDDS("test_savedds.dds", test)
test:saveAsTGA("test_save.tga")
test:saveAsBMP("test_save.bmp")
test:saveAsPNG("test_save.png")
test:saveAsJPG("test_save.jpg")
