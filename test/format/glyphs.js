'use strict';

// Glyph ========================================

exports.Glyph = {read: readGlyph, write: writeGlyph};

function readGlyph(pbf, end) {
    return pbf.readFields(readGlyphField, {}, end);
}

function readGlyphField(tag, glyph, pbf) {
    if (tag === 1) glyph.glyph_index = pbf.readVarint();
    else if (tag === 2) glyph.codepoint = pbf.readVarint();
    else if (tag === 3) glyph.metrics = readGlyphMetrics(pbf, pbf.readVarint() + pbf.pos);
    else if (tag === 4) glyph.bitmap = pbf.readBytes();
}

function writeGlyph(glyph, pbf) {
    if (glyph.glyph_index !== undefined) pbf.writeVarintField(1, glyph.glyph_index);
    if (glyph.codepoint !== undefined) pbf.writeVarintField(2, glyph.codepoint);
    if (glyph.metrics !== undefined) pbf.writeMessage(3, writeGlyphMetrics, glyph.metrics);
    if (glyph.bitmap !== undefined) pbf.writeBytesField(4, glyph.bitmap);
}

// GlyphMetrics ========================================

exports.GlyphMetrics = {read: readGlyphMetrics, write: writeGlyphMetrics};

function readGlyphMetrics(pbf, end) {
    return pbf.readFields(readGlyphMetricsField, {}, end);
}

function readGlyphMetricsField(tag, glyphmetrics, pbf) {
    if (tag === 1) glyphmetrics.width = pbf.readSVarint();
    else if (tag === 2) glyphmetrics.height = pbf.readSVarint();
    else if (tag === 3) glyphmetrics.advance = pbf.readSVarint();
    else if (tag === 4) glyphmetrics.x_bearing = pbf.readSVarint();
    else if (tag === 5) glyphmetrics.y_bearing = pbf.readSVarint();
}

function writeGlyphMetrics(glyphmetrics, pbf) {
    if (glyphmetrics.width !== undefined) pbf.writeSVarintField(1, glyphmetrics.width);
    if (glyphmetrics.height !== undefined) pbf.writeSVarintField(2, glyphmetrics.height);
    if (glyphmetrics.advance !== undefined) pbf.writeSVarintField(3, glyphmetrics.advance);
    if (glyphmetrics.x_bearing !== undefined) pbf.writeSVarintField(4, glyphmetrics.x_bearing);
    if (glyphmetrics.y_bearing !== undefined) pbf.writeSVarintField(5, glyphmetrics.y_bearing);
}

// Face ========================================

exports.Face = {read: readFace, write: writeFace};

function readFace(pbf, end) {
    return pbf.readFields(readFaceField, {"glyphs": []}, end);
}

function readFaceField(tag, face, pbf) {
    if (tag === 1) face.family_name = pbf.readString();
    else if (tag === 2) face.style_name = pbf.readString();
    else if (tag === 3) face.glyphs.push(readGlyph(pbf, pbf.readVarint() + pbf.pos));
    else if (tag === 4) face.metrics = readFaceMetrics(pbf, pbf.readVarint() + pbf.pos);
    else if (tag === 5) face.tables = readFaceTables(pbf, pbf.readVarint() + pbf.pos);
}

function writeFace(face, pbf) {
    if (face.family_name !== undefined) pbf.writeStringField(1, face.family_name);
    if (face.style_name !== undefined) pbf.writeStringField(2, face.style_name);
    var i;
    if (face.glyphs !== undefined) for (i = 0; i < face.glyphs.length; i++) pbf.writeMessage(3, writeGlyph, face.glyphs[i]);
    if (face.metrics !== undefined) pbf.writeMessage(4, writeFaceMetrics, face.metrics);
    if (face.tables !== undefined) pbf.writeMessage(5, writeFaceTables, face.tables);
}

// FaceMetrics ========================================

exports.FaceMetrics = {read: readFaceMetrics, write: writeFaceMetrics};

function readFaceMetrics(pbf, end) {
    return pbf.readFields(readFaceMetricsField, {}, end);
}

function readFaceMetricsField(tag, facemetrics, pbf) {
    if (tag === 1) facemetrics.face_index = pbf.readVarint();
    else if (tag === 2) facemetrics.upem = pbf.readVarint();
    else if (tag === 3) facemetrics.ascender = pbf.readSVarint();
    else if (tag === 4) facemetrics.descender = pbf.readSVarint();
    else if (tag === 5) facemetrics.line_height = pbf.readSVarint();
    else if (tag === 6) facemetrics.line_gap = pbf.readSVarint();
}

function writeFaceMetrics(facemetrics, pbf) {
    if (facemetrics.face_index !== undefined) pbf.writeVarintField(1, facemetrics.face_index);
    if (facemetrics.upem !== undefined) pbf.writeVarintField(2, facemetrics.upem);
    if (facemetrics.ascender !== undefined) pbf.writeSVarintField(3, facemetrics.ascender);
    if (facemetrics.descender !== undefined) pbf.writeSVarintField(4, facemetrics.descender);
    if (facemetrics.line_height !== undefined) pbf.writeSVarintField(5, facemetrics.line_height);
    if (facemetrics.line_gap !== undefined) pbf.writeSVarintField(6, facemetrics.line_gap);
}

// FaceTables ========================================

exports.FaceTables = {read: readFaceTables, write: writeFaceTables};

function readFaceTables(pbf, end) {
    return pbf.readFields(readFaceTablesField, {}, end);
}

function readFaceTablesField(tag, facetables, pbf) {
    if (tag === 1) facetables.gsub = pbf.readBytes();
}

function writeFaceTables(facetables, pbf) {
    if (facetables.gsub !== undefined) pbf.writeBytesField(1, facetables.gsub);
}

// Font ========================================

exports.Font = {read: readFont, write: writeFont};

function readFont(pbf, end) {
    return pbf.readFields(readFontField, {"faces": []}, end);
}

function readFontField(tag, font, pbf) {
    if (tag === 1) font.faces.push(readFace(pbf, pbf.readVarint() + pbf.pos));
    else if (tag === 2) font.metadata = readMetadata(pbf, pbf.readVarint() + pbf.pos);
}

function writeFont(font, pbf) {
    var i;
    if (font.faces !== undefined) for (i = 0; i < font.faces.length; i++) pbf.writeMessage(1, writeFace, font.faces[i]);
    if (font.metadata !== undefined) pbf.writeMessage(2, writeMetadata, font.metadata);
}

// Metadata ========================================

exports.Metadata = {read: readMetadata, write: writeMetadata};

function readMetadata(pbf, end) {
    return pbf.readFields(readMetadataField, {}, end);
}

function readMetadataField(tag, metadata, pbf) {
    if (tag === 1) metadata.size = pbf.readVarint();
    else if (tag === 2) metadata.scale = pbf.readFloat();
    else if (tag === 3) metadata.buffer = pbf.readVarint();
    else if (tag === 4) metadata.radius = pbf.readVarint();
    else if (tag === 5) metadata.offset = pbf.readFloat();
    else if (tag === 6) metadata.cutoff = pbf.readFloat();
    else if (tag === 7) metadata.granularity = pbf.readVarint();
}

function writeMetadata(metadata, pbf) {
    if (metadata.size !== undefined) pbf.writeVarintField(1, metadata.size);
    if (metadata.scale !== undefined) pbf.writeFloatField(2, metadata.scale);
    if (metadata.buffer !== undefined) pbf.writeVarintField(3, metadata.buffer);
    if (metadata.radius !== undefined) pbf.writeVarintField(4, metadata.radius);
    if (metadata.offset !== undefined) pbf.writeFloatField(5, metadata.offset);
    if (metadata.cutoff !== undefined) pbf.writeFloatField(6, metadata.cutoff);
    if (metadata.granularity !== undefined) pbf.writeVarintField(7, metadata.granularity);
}
